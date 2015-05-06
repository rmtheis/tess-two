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
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.ocr.client.Intents;
import com.googlecode.eyesfree.ocr.client.Language;
import com.googlecode.eyesfree.ocr.client.Ocr;

import java.io.File;
import java.util.Collection;
import java.util.List;
import java.util.TreeSet;

/**
 * Displays available and installed languages, allows downloading of new
 * language packs.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class LanguagesActivity extends Activity {
    private static final String TAG = "LanguagesActivity";

    private static final String XML_SOURCE =
            "http://eyes-free.googlecode.com/svn/trunk/thirdparty/tesseract/languages.xml";

    private static final String DATA_SOURCE =
            "http://eyes-free.googlecode.com/svn/trunk/thirdparty/tesseract/";

    private List<Language> mAvailable;

    private LanguageAdapter mAdapter;

    private ListView mListView;

    private File mTessdata;

    private IntentFilter mFilter;

    private IntentFilter mFilter2;

    private SharedPreferences mPrefs;

    private Ocr mOcr;

    private boolean mInitialized = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mFilter = new IntentFilter();
        mFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        mFilter.addDataScheme("file");

        mFilter2 = new IntentFilter();
        mFilter2.addAction(Intents.Actions.LANGUAGES_UPDATED);

        mPrefs = getPreferences(MODE_PRIVATE);

        mAdapter = new LanguageAdapter(this);
        mAdapter.setNotifyOnChange(true);

        Ocr.InitCallback initCallback = new Ocr.InitCallback() {
            @Override
            public void onInitialized(int status) {
                updateLanguages();

                mInitialized = true;
            }
        };

        mOcr = new Ocr(this, initCallback, true);

        onConfigurationChanged(getResources().getConfiguration());
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);

        setContentView(R.layout.manage);

        mListView = (ListView) findViewById(R.id.list_languages);
        mListView.setAdapter(mAdapter);
        mListView.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        mListView.setOnItemClickListener(itemClickListener);
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (mInitialized) {
            updateLanguages();
        }

        registerReceiver(broadcastReceiver, mFilter);
        registerReceiver(broadcastReceiver, mFilter2);
    }

    @Override
    protected void onPause() {
        unregisterReceiver(broadcastReceiver);

        super.onPause();
    }

    @Override
    public void onDestroy() {
        mOcr.release();

        super.onDestroy();
    }

    private void updateLanguages() {
        mTessdata = mOcr.getTessdata();
        mAvailable = mOcr.getAvailableLanguages();

        final String xmlPref = getString(R.string.xml_cache_pref);
        final String cachedXml = mPrefs.getString(xmlPref, null);

        new XmlLoader(this, mAvailable) {
            @Override
            protected void onPostExecute(final TreeSet<LanguageData> result) {
                // Show results on-screen
                onLoad(result);

                // Cache resulting XML in preferences
                if (result != null) {
                    final String cachedXml = getCachedXml();

                    if (cachedXml != null) {
                        final SharedPreferences.Editor editor = mPrefs.edit();
                        editor.putString(xmlPref, cachedXml);
                        editor.commit();
                    }
                }

                super.onPostExecute(result);
            }
        }.setCachedXml(cachedXml).execute(XML_SOURCE);
    }

    private void onLoad(TreeSet<LanguageData> languages) {
        if (languages == null) {
            Toast.makeText(this, R.string.manage_error, Toast.LENGTH_LONG).show();
            return;
        }

        mAdapter.clear();
        mAdapter.addAllOf(languages);

    }

    private void showConfirm(String text, DialogInterface.OnClickListener onClick) {
        Dialog dialog = new AlertDialog.Builder(this).setCancelable(true).setMessage(text)
                .setPositiveButton(getString(android.R.string.yes), onClick)
                .setNegativeButton(getString(android.R.string.cancel), null).create();

        dialog.setOwnerActivity(this);
        dialog.show();
    }

    private void installLanguage(final LanguageData data) {
        Log.i(TAG, "Installing " + data.name + "...");

        new LanguageLoader(this, DATA_SOURCE, mTessdata) {
            @Override
            protected void onPostExecute(final LanguageData result) {
                // Show results on-screen
                onInstall(result, getCanceled());

                super.onPostExecute(result);
            }
        }.execute(data);
    }

    private void onInstall(LanguageData data, boolean canceled) {
        int resId;

        if (data != null) {
            data.installed = true;
            mAdapter.notifyDataSetChanged();
            mOcr.reloadLanguages();

            resId = R.string.install_completed;
        } else if (canceled) {
            resId = R.string.install_canceled;
        } else {
            resId = R.string.install_failed;
        }

        String message = getString(resId, data.name);
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    private void promptUninstallLanguage(final LanguageData data) {
        DialogInterface.OnClickListener onClickListener = new DialogInterface.OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                if (which == DialogInterface.BUTTON_POSITIVE) {
                    Thread uninstaller = new Thread() {
                        @Override
                        public void run() {
                            uninstallLanguageAsync(data);
                        }
                    };
                    uninstaller.start();
                }
            }
        };

        String message = getString(R.string.uninstall_confirm, data.name);
        showConfirm(message, onClickListener);
    }

    private void uninstallLanguageAsync(final LanguageData data) {
        final File installed = new File(mTessdata, data.iso6392 + ".traineddata");
        final boolean success = installed.delete();

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                onUninstall(data, success);
            }
        });
    }

    private void onUninstall(LanguageData data, boolean success) {
        int resId;

        if (success) {
            data.installed = false;
            mAdapter.notifyDataSetChanged();
            mOcr.reloadLanguages();

            resId = R.string.uninstall_completed;
        } else {
            resId = R.string.uninstall_failed;
        }

        String message = getString(resId, data.name);
        Toast.makeText(this, message, Toast.LENGTH_LONG).show();
    }

    private class LanguageAdapter extends ArrayAdapter<LanguageData> {
        private final int mColorRed;

        public LanguageAdapter(Context context) {
            super(context, 0);

            mColorRed = context.getResources().getColor(R.color.red);
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            LanguageData data = getItem(position);

            LayoutInflater inflate = LayoutInflater.from(getContext());
            View view = inflate.inflate(R.layout.language, null);
            ((TextView) view.findViewById(R.id.text_size)).setText(data.size);
            ((TextView) view.findViewById(R.id.text_iso6392)).setText(data.iso6392);
            ((TextView) view.findViewById(R.id.text_name)).setText(data.name);

            TextView installed = ((TextView) view.findViewById(R.id.text_installed));
            installed.setText(data.installed ? R.string.installed : R.string.not_installed);

            if (!data.installed) {
                installed.setTextColor(mColorRed);
            }

            return view;
        }

        public void addAllOf(Collection<? extends LanguageData> items) {
            for (LanguageData item : items) {
                add(item);
            }
        }
    }

    static class LanguageData implements Comparable<LanguageData> {
        String size;
        String file;
        String iso6392;
        String name;

        boolean installed;
        boolean hidden;

        @Override
        public int compareTo(LanguageData another) {
            return name.compareTo(another.name);
        }
    }

    private final OnItemClickListener itemClickListener = new OnItemClickListener() {
        @Override
        public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
            LanguageData data = mAdapter.getItem(position);

            if (data == null) {
                return;
            }

            if (!data.installed) {
                installLanguage(data);
            } else {
                promptUninstallLanguage(data);
            }
        }
    };

    private final BroadcastReceiver broadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
                finish();
            } else if (mInitialized
                    && intent.getAction().equals(Intents.Actions.LANGUAGES_UPDATED)) {
                updateLanguages();
            }
        }
    };
}
