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

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Environment;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceActivity;
import android.util.Log;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.ocr.client.Intents;
import com.googlecode.eyesfree.ocr.client.Language;
import com.googlecode.eyesfree.ocr.client.Ocr;

import java.util.List;

/**
 * Provides access to global OCR preferences.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class PrefsActivity extends PreferenceActivity {
    private static final String TAG = "PrefsActivity";

    private Preference mManagePref;
    private ListPreference mLangPref;
    private IntentFilter mFilter;

    private String mStrManage;
    private String mStrLang;

    private Ocr mOcr;

    private boolean mInitialized;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        addPreferencesFromResource(R.xml.prefs);

        mStrManage = getString(R.string.manage_pref);
        mStrLang = getString(R.string.lang_pref);

        // "Test Activities" is disabled for now...
        // mStrTest = getString(R.string.test_pref);
        // findPreference(mStrTest).setOnPreferenceClickListener(onPreferenceClick);

        mManagePref = findPreference(mStrManage);
        mManagePref.setOnPreferenceClickListener(onPreferenceClick);
        mManagePref.setEnabled(false);

        mLangPref = (ListPreference) findPreference(mStrLang);
        mLangPref.setEnabled(false);

        mFilter = new IntentFilter();
        mFilter.addAction(Intents.Actions.LANGUAGES_UPDATED);

        Ocr.InitCallback initCallback = new Ocr.InitCallback() {
            @Override
            public void onInitialized(int status) {
                updateLanguages();

                mInitialized = true;
            }
        };

        mInitialized = false;
        mOcr = new Ocr(this, initCallback);
    }

    @Override
    protected void onResume() {
        super.onResume();

        registerReceiver(broadcastReceiver, mFilter);

        if (mInitialized) {
            updateLanguages();
        }
    }

    @Override
    protected void onPause() {
        unregisterReceiver(broadcastReceiver);

        super.onPause();
    }

    @Override
    protected void onDestroy() {
        mOcr.release();

        super.onDestroy();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);

        if (hasFocus) {
            updateLangPrefSummary();
        }
    }

    private void updateLangPrefSummary() {
        final String defaultLanguage = mLangPref.getValue();

        if (defaultLanguage != null) {
            mLangPref.setSummary(getString(R.string.lang_pref_summary, defaultLanguage));
        }
    }

    private void updateLanguages() {
        // Abort if the SD card isn't inserted
        if (!Environment.getExternalStorageDirectory().canRead()) {
            Log.e(TAG, "Missing SD card");

            mLangPref.setEnabled(false);
            mManagePref.setEnabled(false);
            return;
        }

        List<Language> available = mOcr.getAvailableLanguages();

        // Enable language management and abort if we're missing languages
        if (available.isEmpty()) {
            Log.e(TAG, "No languages installed");

            mLangPref.setEnabled(false);
            mManagePref.setEnabled(true);
            return;
        }

        String[] entries = new String[available.size()];
        String[] values = new String[available.size()];

        for (int i = 0; i < available.size(); i++) {
            Language lang = available.get(i);

            values[i] = lang.iso_639_2;
            entries[i] = lang.english;
        }

        mLangPref.setEntries(entries);
        mLangPref.setEntryValues(values);
        mLangPref.setEnabled(true);
        mManagePref.setEnabled(true);
    }

    private final OnPreferenceClickListener onPreferenceClick = new OnPreferenceClickListener() {
        @Override
        public boolean onPreferenceClick(Preference preference) {
            String key = preference.getKey();
            Intent intent;

            if (mStrManage.equals(key)) {
                intent = new Intent(Intents.Languages.ACTION);
            } else {
                return false;
            }

            startActivity(intent);

            return true;
        }
    };

    private final BroadcastReceiver broadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (mInitialized && intent.getAction().equals(Intents.Actions.LANGUAGES_UPDATED)) {
                updateLanguages();
            }
        }
    };
}
