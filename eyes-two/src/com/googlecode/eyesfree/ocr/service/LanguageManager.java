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

package com.googlecode.eyesfree.ocr.service;

import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.ocr.client.Intents;
import com.googlecode.eyesfree.ocr.client.Language;

import java.io.File;
import java.io.FilenameFilter;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * Provides a list of installed languages.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class LanguageManager {
    private static final String TESSDATA = "tessdata";
    private static final String EXTENSION = ".traineddata";
    private static final String ENGLISH_UNKNOWN = "Unknown (%s)";
    private static final String ISO6391_UNKNOWN = "??";

    private File mDatapath;
    private File mTessdata;

    private Map<String, Language> mSupported;
    private List<Language> mAvailable;
    private WeakReference<Context> mContext;

    public LanguageManager(Context context) {
        mContext = new WeakReference<Context>(context);
        mSupported = new TreeMap<String, Language>();
        mAvailable = new ArrayList<Language>();
    }

    public boolean loadLanguages() {
        Context context = mContext.get();

        if (context == null)
            return false;

        mDatapath = context.getExternalFilesDir(null);

        if (mDatapath == null) {
            return false;
        }

        mTessdata = new File(mDatapath, TESSDATA);

        loadSupported();
        loadAvailable();

        // Broadcast intent to let everyone know we've updated the language list
        Intent intent = new Intent(Intents.Actions.LANGUAGES_UPDATED);
        context.sendBroadcast(intent);

        return true;
    }

    public Map<String, Language> getSupported() {
        return mSupported;
    }

    /**
     * Returns the list of available languages. You must call loadLanguages()
     * prior to calling this method.
     *
     * @return the list of available languages
     */
    public List<Language> getAvailable() {
        return mAvailable;
    }

    /**
     * Returns the directory containing tessdata. You must call loadLanguages()
     * prior to calling this method.
     *
     * @return the directory containing tessdata
     */
    public File getDatapath() {
        return mDatapath;
    }

    /**
     * Returns the tessdata directory. You must call loadLanguages() prior to
     * calling this method.
     *
     * @return the tessdata directory
     */
    public File getTessdata() {
        return mTessdata;
    }

    // *******************
    // * Private methods *
    // *******************

    private void loadSupported() {
        mSupported.clear();

        Context context = mContext.get();

        if (context == null)
            return;

        Resources res = context.getResources();

        String[] english = res.getStringArray(R.array.english);
        String[] iso6391 = res.getStringArray(R.array.iso_639_1);
        String[] iso6392 = res.getStringArray(R.array.iso_639_2);

        for (int i = 0; i < english.length; i++) {
            Language lang = new Language(english[i], iso6391[i], iso6392[i]);

            mSupported.put(iso6392[i], lang);
        }
    }

    private void loadAvailable() {
        mAvailable.clear();

        if (mTessdata == null)
            return;

        String[] files = mTessdata.list(trainedDataFilter);

        if (files == null)
            return;

        for (String file : files) {
            int lastIndex = file.lastIndexOf('.');
            String iso6392 = file.substring(0, lastIndex);
            Language lang = mSupported.get(iso6392);

            if (lang == null) {
                String english = String.format(ENGLISH_UNKNOWN, iso6392);
                lang = new Language(english, ISO6391_UNKNOWN, iso6392);
            }

            mAvailable.add(lang);
        }

        Collections.sort(mAvailable);
    }

    // *******************
    // * Private classes *
    // *******************

    private FilenameFilter trainedDataFilter = new FilenameFilter() {
        @Override
        public boolean accept(File dir, String filename) {
            return filename.endsWith(EXTENSION);
        }
    };
}
