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

package com.googlecode.eyesfree.ocr.intent;

import android.app.Activity;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.os.AsyncTask;
import android.util.Log;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.ocr.intent.LanguagesActivity.LanguageData;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * @author alanv@google.com (Alan Viverette)
 */
public class LanguageLoader extends AsyncTask<LanguageData, Object, LanguageData> {
    private static final String TAG = "LanguageLoader";

    private final String mDownloading;

    private final String mExtracting;

    private final String mDataSource;

    private final File mTargetFolder;

    private boolean mCanceled;

    private ProgressDialog mDialog;

    public LanguageLoader(Activity activity, String dataSource, File targetFolder) {
        mDownloading = activity.getString(R.string.downloading);
        mExtracting = activity.getString(R.string.extracting);
        mDataSource = dataSource;
        mTargetFolder = targetFolder;
        mCanceled = false;

        String message = activity.getString(R.string.manage_extracting);

        mDialog = new ProgressDialog(activity);
        mDialog.setMax(100);
        mDialog.setProgress(0);
        mDialog.setIndeterminate(false);
        mDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mDialog.setMessage(message);
        mDialog.setCancelable(true);
        mDialog.setOwnerActivity(activity);
        mDialog.setOnCancelListener(new ProgressDialog.OnCancelListener() {
            @Override
            public void onCancel(DialogInterface dialog) {
                mCanceled = true;
            }
        });
    }

    protected boolean getCanceled() {
        return mCanceled;
    }

    @Override
    protected void onPreExecute() {
        mDialog.show();
    }

    @Override
    protected void onProgressUpdate(Object... progress) {
        for (Object update : progress) {
            if (update instanceof CharSequence) {
                mDialog.setMessage((CharSequence) update);
            } else if (update instanceof Integer) {
                mDialog.setProgress((Integer) update);
            }
        }
    }

    @Override
    protected void onPostExecute(LanguageData result) {
        mDialog.dismiss();
    }

    @Override
    protected LanguageData doInBackground(LanguageData... params) {
        if (!mTargetFolder.exists() && !mTargetFolder.mkdirs()) {
            return null;
        }

        LanguageData result = null;

        try {
            result = download(params[0]);
        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }

        return result;
    }

    private LanguageData download(LanguageData data) throws MalformedURLException, IOException {
        final URL fileUrl = new URL(mDataSource + data.file);
        final URLConnection urlConn = fileUrl.openConnection();
        final ZipInputStream zipStream = new ZipInputStream(urlConn.getInputStream());
        final byte buffer[] = new byte[200000];

        final String packMessage = String.format(
                mDownloading, fileUrl.getFile(), fileUrl.getHost());
        publishProgress(packMessage, 0);

        ZipEntry entry;

        while (!mCanceled && (entry = zipStream.getNextEntry()) != null) {
            final File outFile = new File(mTargetFolder, entry.getName());

            Log.i(TAG, "Extracting " + entry.getName());

            if (entry.isDirectory()) {
                outFile.mkdir();
                continue;
            } else {
                outFile.createNewFile();
            }

            final int maxSize = (int) entry.getSize();
            final String fileMessage = String.format(mExtracting, outFile.getName(), data.file);
            publishProgress(fileMessage, 0);

            FileOutputStream out = new FileOutputStream(outFile);

            int progress = 0;
            int readBytes;

            while (!mCanceled && (readBytes = zipStream.read(buffer, 0, buffer.length)) > 0) {
                out.write(buffer, 0, readBytes);

                progress += readBytes;

                int percentProgress = 100 * progress / maxSize;
                publishProgress(percentProgress);
            }

            out.close();

            if (mCanceled) {
                outFile.delete();
            }
        }

        zipStream.close();

        if (mCanceled) {
            return null;
        } else {
            return data;
        }
    }

}
