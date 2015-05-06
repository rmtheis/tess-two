/*
 * Copyright (C) 2011 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

package com.googlecode.eyesfree.ocr.client;

import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Bitmap;
import android.graphics.Bitmap.CompressFormat;
import android.os.Bundle;
import android.os.DeadObjectException;
import android.os.Environment;
import android.os.IBinder;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.RemoteException;
import android.util.Log;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.List;
import java.util.Set;

/**
 * Recognizes text in images. This abstracts away the complexities of using the
 * OCR service such as setting up the IBinder connection and handling
 * RemoteExceptions, etc. Specifically, this class initializes the OCR service
 * and pushes recognization requests across IPC for processing in the service
 * thread.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Ocr {
    private static final String TAG = "Ocr";

    // This is the minimum version of the Ocr service that is needed by this
    // version of the library stub.
    private static final int MIN_VER = 1;

    public static final int STATUS_SUCCESS = 0;
    public static final int STATUS_FAILURE = 1;
    public static final int STATUS_MISSING = 2;

    public static final int ERROR = -1;
    public static final int SUCCESS = 1;

    public static final long INVALID_TOKEN = -1;

    private static final int BINDER_SIZE_LIMIT = 40000;

    private int mVersion = -1;

    private IOcr mIOcr;

    private ServiceConnection mServiceConnection;

    private boolean mStorageAvailable;

    private boolean mSuppressAlerts;

    private WeakReference<Context> mContext;

    private ResultCallback mOnResult;

    private CompletionCallback mOnCompleted;

    private Parameters mParameters;

    /**
     * The constructor for the OCR service client. Initializes the service if
     * necessary and calls the supplied InitCallback when it's ready.
     *
     * @param context the context of the parent activity
     * @param init the callback to call on initialization
     */
    public Ocr(Context context, InitCallback init) {
        this(context, init, false);
    }

    /**
     * The constructor for the OCR service client. Initializes the service if
     * necessary and calls the supplied InitCallback when it's ready.
     * <p>
     * You may optionally set suppressAlerts to true to turn off alert dialogs.
     *
     * @param context the context of the parent activity
     * @param init the callback to call on initialization
     * @param suppressAlerts <code>true</code> to suppress alert dialogs
     */
    public Ocr(Context context, InitCallback init, boolean suppressAlerts) {
        if (context == null) {
            throw new IllegalArgumentException("Context must not be null");
        }

        mContext = new WeakReference<Context>(context);
        mSuppressAlerts = suppressAlerts;
        mParameters = new Parameters();

        connectOcrService(init);
    }

    /**
     * Sets the result callback. If text detection is enabled, this will be
     * called once for each individual box before the completion callback
     * occurs.
     *
     * @param callback
     */
    public void setResultCallback(ResultCallback callback) {
        mOnResult = callback;
    }

    /**
     * Sets the completion callback. This is called when recognition is complete
     * and receives an ArrayList of results.
     *
     * @param callback
     */
    public void setCompletionCallback(CompletionCallback callback) {
        mOnCompleted = callback;
    }

    /**
     * Enqueues an image represented as a Bitmap for OCR.
     *
     * @param bitmap The bitmap on which to perform OCR.
     * @return A Job representing the queued OCR job.
     */
    public Job enqueue(Bitmap bitmap) {
        if (bitmap == null) {
            throw new IllegalArgumentException("Bitmap must be non-null");
        }

        // TODO(alanv): Replace this with native Bitmap conversion
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        bitmap.compress(CompressFormat.JPEG, 85, byteStream);

        byte[] jpegData = byteStream.toByteArray();

        return enqueue(jpegData);
    }

    /**
     * Enqueues an image represented as JPEG-compressed bytes for OCR.
     *
     * @param jpegData The JPEG-compressed image on which to perform OCR.
     * @return A Job representing the queued OCR job.
     */
    public Job enqueue(byte[] jpegData) {
        if (jpegData == null) {
            throw new IllegalArgumentException("JPEG data must be non-null");
        }

        // If we're over the binder size limit, write to disk.
        if (jpegData.length > BINDER_SIZE_LIMIT) {
            return cacheAndEnqueue(jpegData);
        }

        try {
            long taskId = mIOcr.enqueueData(jpegData, mParameters);
            return new Job(taskId);
        } catch (DeadObjectException e) {
            e.printStackTrace();
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        return null;
    }

    /**
     * Internal method that writes image bytes to disk when they exceed the
     * binder transaction limit.
     *
     * @param data The bytes to write to disk.
     * @return A Job representing the queued OCR job.
     */
    private Job cacheAndEnqueue(byte[] data) {
        Job job = null;

        try {
            File cacheDir = mContext.get().getExternalCacheDir();
            File cached = File.createTempFile("ocr", ".jpg", cacheDir);

            FileOutputStream output = new FileOutputStream(cached);
            output.write(data);
            output.close();

            job = enqueue(cached);

            if (job != null) {
                job.mCached = cached;
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        return job;
    }

    /**
     * Enqueues an image represented as an encoded file. The file extension must
     * match the encoding and must be one of the following formats:
     * <ol>
     * <li>JPEG</li>
     * <li>BMP</li>
     * </ol>
     *
     * @param file An encoded file containing the image to OCR.
     * @return A Job representing the queued OCR job.
     */
    public Job enqueue(File file) {
        if (file == null) {
            throw new IllegalArgumentException("File must be non-null");
        }

        try {
            long taskId = mIOcr.enqueueFile(file.getAbsolutePath(), mParameters);
            return new Job(taskId);
        } catch (DeadObjectException e) {
            e.printStackTrace();
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        return null;
    }

    /**
     * Returns the OCR parameters that will be used to process new enqueue
     * requests. If changes are made, you must call setParameters to commit
     * them.
     *
     * @return The parameters used when processing new OCR requests.
     */
    public Parameters getParameters() {
        return mParameters;
    }

    /**
     * Sets the OCR parameters that will be used to process new enqueue
     * requests.
     *
     * @param parameters The parameters to use when processing new OCR requests.
     */
    public void setParameters(Parameters parameters) {
        mParameters = parameters;
    }

    /**
     * Returns the absolute path of the OCR service's language data folder.
     * Typically this is on the user's SD card.
     *
     * @return the absolute path of the OCR service's language data folder
     */
    public File getTessdata() {
        if (mIOcr == null) {
            Log.e(TAG, "getTessdata() without a connection to Ocr service.");
            return null;
        }

        File tessdata = null;

        try {
            String tessstr = mIOcr.getTessdata();

            tessdata = tessstr == null ? null : new File(tessstr);
        } catch (DeadObjectException e) {
            e.printStackTrace();
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        return tessdata;
    }

    /**
     * Forces the Ocr service to refresh the list of available languages.
     */
    public void reloadLanguages() {
        if (mIOcr == null) {
            Log.e(TAG, "reloadLanguages() without a connection to Ocr service.");
        }

        try {
            mIOcr.reloadLanguages();
        } catch (DeadObjectException e) {
            e.printStackTrace();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /**
     * Returns the list of available languages.
     *
     * @return a sorted list of available languages
     */
    public List<Language> getAvailableLanguages() {
        if (mIOcr == null) {
            Log.e(TAG, "getAvailableLanguages() without a connection to Ocr service.");
            return null;
        }

        List<Language> available = null;

        try {
            available = mIOcr.getAvailableLanguages();
        } catch (DeadObjectException e) {
            e.printStackTrace();
        } catch (RemoteException e) {
            e.printStackTrace();
        }

        return available;
    }

    /**
     * Disconnects from the Ocr service.
     * <p>
     * It is recommended that you call this as soon as you're done with the Ocr
     * object. After this call the receiving Ocr object will be unusable.
     */
    public synchronized void release() {
        mOnCompleted = null;
        mOnResult = null;

        try {
            Context context = mContext.get();

            if (context != null) {
                context.unbindService(mServiceConnection);
            }
        } catch (IllegalArgumentException e) {
            // Do nothing and fail silently since an error here indicates that
            // binding never succeeded in the first place.
        }

        mIOcr = null;
        mContext = null;
    }

    /**
     * Internal method used to connect to the OCR service.
     *
     * @param init Initialization callback.
     */
    private void connectOcrService(final InitCallback init) {
        // Initialize the OCR service, run the callback after the binding is
        // successful
        mServiceConnection = new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                mIOcr = IOcr.Stub.asInterface(service);

                try {
                    mVersion = mIOcr.getVersion();

                    // The Ocr service must be at least the min version needed
                    // by the library stub. Do not try to run the older Ocr with
                    // the newer library stub as the newer library may reference
                    // methods which are unavailable and cause a crash.

                    if (mVersion < MIN_VER) {
                        Log.e(TAG, "OCR service too old (version " + mVersion + " < " + MIN_VER
                                + ")");

                        if (!mSuppressAlerts) {
                            OnClickListener onClick = new OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    postInitialized(init, STATUS_MISSING);
                                }
                            };

                            VersionAlert.createUpdateAlert(mContext.get(), null).show();
                        } else {
                            postInitialized(init, STATUS_MISSING);
                        }

                        return;
                    }

                    mStorageAvailable = Environment.getExternalStorageDirectory().exists();

                    if (!mStorageAvailable) {
                        Log.e(TAG, "External storage is not available");

                        if (!mSuppressAlerts) {
                            OnClickListener onClick = new OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    postInitialized(init, STATUS_MISSING);
                                }
                            };

                            VersionAlert.createStorageAlert(mContext.get(), onClick).show();
                        } else {
                            postInitialized(init, STATUS_MISSING);
                        }

                        return;
                    }

                    List<Language> languages = mIOcr.getAvailableLanguages();

                    if (languages == null || languages.isEmpty()) {
                        Log.e(TAG, "No languages are installed");

                        if (!mSuppressAlerts) {
                            OnClickListener onClick = new OnClickListener() {
                                @Override
                                public void onClick(DialogInterface dialog, int which) {
                                    postInitialized(init, STATUS_MISSING);
                                }
                            };

                            VersionAlert.createLanguagesAlert(mContext.get(), onClick, onClick)
                                    .show();
                        } else {
                            postInitialized(init, STATUS_MISSING);
                        }

                        return;
                    }

                    // Set the callback so that we can receive completion events
                    mIOcr.setCallback(mCallback);

                } catch (RemoteException e) {
                    Log.e(TAG, "Exception caught in onServiceConnected(): " + e.toString());

                    postInitialized(init, STATUS_FAILURE);

                    return;
                }

                postInitialized(init, STATUS_SUCCESS);
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
                mIOcr = null;
            }
        };

        Intent intent = new Intent(Intents.Service.ACTION);
        intent.addCategory(Intent.CATEGORY_DEFAULT);

        // Binding will fail only if the Ocr doesn't exist;
        // the OcrVersionAlert will give users a chance to install
        // the needed Ocr.

        Context context = mContext.get();

        if (!context.bindService(intent, mServiceConnection, Context.BIND_AUTO_CREATE)) {
            Log.e(TAG, "Cannot bind to OCR service, assuming not installed");

            OnClickListener onClick = new OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    postInitialized(init, STATUS_MISSING);
                }
            };

            if (!mSuppressAlerts) {
                VersionAlert.createInstallAlert(context, onClick).show();
            }

            return;
        }
    }

    /**
     * Passes the initialization status to the InitCallback.
     *
     * @param init The initialization callback.
     * @param status The initialization status.
     */
    private void postInitialized(final InitCallback init, final int status) {
        if (init != null) {
            init.onInitialized(status);
        }
    }

    /**
     * Cancels all active and pending OCR jobs.
     */
    public void stop() {
        if (mIOcr == null) {
            Log.e(TAG, "Attempted to call stop() without a connection to Ocr service.");
            return;
        }

        try {
            mIOcr.stop();
        } catch (DeadObjectException e) {
            e.printStackTrace();
        } catch (RemoteException e) {
            e.printStackTrace();
        }
    }

    /**
     * Returns the version number of the Ocr library that the user has
     * installed.
     *
     * @return te version number of the Ocr library that the user has installed
     */
    public int getVersion() {
        return mVersion;
    }

    /**
     * Checks if the Ocr service is installed or not
     *
     * @return a boolean that indicates whether the Ocr service is installed
     */
    public static boolean isInstalled(Context ctx) {
        Intent intent = new Intent(Intents.Service.ACTION);

        PackageManager pm = ctx.getPackageManager();
        ResolveInfo info = pm.resolveService(intent, 0);

        if (info == null) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * Handles the callback when the Ocr service has initialized.
     */
    public static interface InitCallback {
        public void onInitialized(int status);
    }

    /**
     * Handles the callback for when recognition is completed.
     */
    public static interface CompletionCallback {
        public void onCompleted(List<OcrResult> results);
    }

    /**
     * Handles the callback for a single mid-recognition result.
     */
    public static interface ResultCallback {
        public void onResult(OcrResult result);
    }

    private final IOcrCallback mCallback = new IOcrCallback.Stub() {
        @Override
        public void onCompleted(final long token, final List<OcrResult> results) {
            if (mOnCompleted != null) {
                mOnCompleted.onCompleted(results);
            }
        }

        @Override
        public void onResult(final long token, final OcrResult result) {
            if (mOnResult != null) {
                mOnResult.onResult(result);
            }
        }
    };

    /**
     * Represents a single OCR job.
     *
     * @author alanv@google.com (Alan Viverette)
     */
    public class Job {
        long mTaskId;

        File mCached;

        Job(long taskId) {
            mTaskId = taskId;
            mCached = null;
        }

        @Override
        protected void finalize() throws Throwable {
            // If we have a cached file, delete it when we're done.
            try {
                if (mCached != null) {
                    mCached.delete();
                }
            } finally {
                super.finalize();
            }
        }

        /**
         * Cancels this OCR job.
         */
        public void cancel() {
            try {
                mIOcr.cancel(mTaskId);
            } catch (DeadObjectException e) {
                e.printStackTrace();
            } catch (RemoteException e) {
                e.printStackTrace();
            }
        }
    }

    /**
     * Represents a set of OCR processing parameters.
     *
     * @author alanv@google.com (Alan Viverette)
     */
    public static class Parameters implements Parcelable {
        /** Whitelist of characters to recognize */
        public static final String VAR_CHAR_WHITELIST = "tessedit_char_whitelist";

        /** Blacklist of characters to not recognize */
        public static final String VAR_CHAR_BLACKLIST = "tessedit_char_blacklist";

        /** Detect text in image using TextDetect */
        public static final String FLAG_DETECT_TEXT = "detect_text";

        /** Aligns horizontal text in an image */
        public static final String FLAG_ALIGN_TEXT = "align_text";

        /** Perform spell-checking on results */
        public static final String FLAG_SPELLCHECK = "spellcheck";

        /** Write intermediate files to external storage */
        public static final String FLAG_DEBUG_MODE = "debug_mode";

        /** Fully automatic page segmentation. */
        public static final int PSM_AUTO = 0;

        /** Assume a single column of text of variable sizes. */
        public static final int PSM_SINGLE_COLUMN = 1;

        /** Assume a single uniform block of text. */
        public static final int PSM_SINGLE_BLOCK = 2;

        /** Treat the image as a single text line. (Default) */
        public static final int PSM_SINGLE_LINE = 3;

        /** Treat the image as a single word. */
        public static final int PSM_SINGLE_WORD = 4;

        /** Treat the image as a single character. */
        public static final int PSM_SINGLE_CHAR = 5;

        private static final int PSM_MODE_COUNT = 6;

        private Bundle mVariables;

        private Bundle mFlags;

        private String mLanguage;

        private int mPageSegMode;

        /**
         * Constructs a new Parameters object using the default values.
         */
        public Parameters() {
            mVariables = new Bundle();
            mFlags = new Bundle();
            mPageSegMode = PSM_SINGLE_LINE;
            mLanguage = "eng";
        }

        /**
         * Sets the value of the variable identified by <code>key</code>. If the
         * value is null, removes the variable.
         *
         * @param key The key that identifies the variable to set.
         * @param value The String value to assign to the variable.
         */
        public void setVariable(String key, String value) {
            if (value == null) {
                mVariables.remove(key);
            } else {
                mVariables.putString(key, value);
            }
        }

        /**
         * Returns the value of the variable identified by <code>key</code>, or
         * <code>null</code> if it has not been set.
         *
         * @param key The key that identifies the variable to retrieve.
         * @return The value of the variable or <code>null</code> if it has not
         *         been set.
         */
        public String getVariable(String key) {
            return mVariables.getString(key);
        }

        /**
         * Returns the list of keys identifying variables that have been set.
         *
         * @return A set of Strings representing the variable keys that have
         *         been set.
         */
        public Set<String> getVariableKeys() {
            return mVariables.keySet();
        }

        /**
         * Sets the value of the flag identified by <code>key</code>. If the
         * value is <code>null</code>, removes the flag.
         *
         * @param key The key that identifies the flag to set.
         * @param value The boolean value to assign to the flag.
         */
        public void setFlag(String key, boolean value) {
            mFlags.putBoolean(key, value);
        }

        /**
         * Returns the value of the flag identified by <code>key</code>. If
         * <code>key</code> has not been set, returns <code>false</code>.
         *
         * @param key The key that identifies the flag to retrieve.
         * @return The value of the flag or <code>false</code> if it has not
         *         been set.
         */
        public boolean getFlag(String key) {
            if (!mFlags.containsKey(key)) {
                return false;
            } else {
                return mFlags.getBoolean(key);
            }
        }

        /**
         * Sets the language used by the OCR engine. Use
         * Ocr.getAvailableLanguages() to retrieve the list of available
         * languages.
         *
         * @param language A language present in Ocr.getAvailableLanguages().
         */
        public void setLanguage(Language language) {
            mLanguage = language.iso_639_2;
        }

        /**
         * Sets the language (as an ISO 639-2 code) used by the OCR engine. Use
         * Ocr.getAvailableLanguages() to retrieve the list of available
         * languages and Language.iso_639_2 to retrieve the ISO 639-2 code. If
         * the specified language is not available, the OCR engine will default
         * to English.
         *
         * @param language An ISO 639-2 code representing a supported language.
         */
        public void setLanguage(String language) {
            mLanguage = language;
        }

        /**
         * Returns the ISO 639-2 code representing the current language that
         * will be used by the OCR engine.
         *
         * @return The ISO 639-2 code representing the current languages used
         *         for OCR.
         */
        public String getLanguage() {
            return mLanguage;
        }

        /**
         * Sets the page segmentation mode, which is used by the OCR engine to
         * detect and group areas of text. See the Parameters.PSM_* constants
         * for available values.
         *
         * @param pageSegMode A page segmentation mode from Parameters.PSM_*
         *            constants.
         */
        public void setPageSegMode(int pageSegMode) {
            if (pageSegMode < 0 || pageSegMode > PSM_MODE_COUNT) {
                throw new IllegalArgumentException("Invalid page segmentation mode");
            }

            mPageSegMode = pageSegMode;
        }

        /**
         * Returns the current page segmentation mode as defined in
         * Parameters.PSM_* constants.
         *
         * @return The current page segmentation mode.
         */
        public int getPageSegMode() {
            return mPageSegMode;
        }

        // ************************
        // * Parcelable functions *
        // ************************

        private Parameters(Parcel src) {
            readFromParcel(src);
        }

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeBundle(mVariables);
            dest.writeBundle(mFlags);
            dest.writeString(mLanguage);
        }

        private void readFromParcel(Parcel src) {
            mVariables = src.readBundle();
            mFlags = src.readBundle();
            mLanguage = src.readString();
        }

        public static final Parcelable.Creator<Parameters> CREATOR = new Parcelable.Creator<Parameters>() {
            @Override
            public Parameters createFromParcel(Parcel in) {
                return new Parameters(in);
            }

            @Override
            public Parameters[] newArray(int size) {
                return new Parameters[size];
            }
        };
    }
}
