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

import android.os.Parcel;
import android.os.Parcelable;

/**
 * This class represents a language with three different interpretations:
 * <ol>
 * <li>An English name
 * <li>An ISO 639-1 code, as used by Google Translate
 * <li>An ISO 639-2 code, as used by Tesseract
 * </ol>
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class Language implements Comparable<Language>, Parcelable {
    /** The English representation of this language. */
    public String english;

    /**
     * The ISO 639-1 representation of this language (as used by Google
     * Translate).
     */
    public String iso_639_1;

    /** The ISO 639-2 representation of this language (as used by Tesseract). */
    public String iso_639_2;

    /**
     * Constructs a new language with an English name, an ISO 639-1 code, and an
     * ISO 639-2 code.
     *
     * @param en The English name of the language.
     * @param i1 The language's ISO 639-1 code.
     * @param i2 The language's ISO 639-2 code.
     */
    public Language(String en, String i1, String i2) {
        english = en;
        iso_639_1 = i1;
        iso_639_2 = i2;
    }

    @Override
    public int compareTo(Language another) {
        return english.compareTo(another.english);
    }

    @Override
    public String toString() {
        return english;
    }

    /************************
     * Parcelable functions *
     ************************/

    private Language(Parcel src) {
        readFromParcel(src);
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(english);
        dest.writeString(iso_639_1);
        dest.writeString(iso_639_2);
    }

    private void readFromParcel(Parcel src) {
        english = src.readString();
        iso_639_1 = src.readString();
        iso_639_2 = src.readString();
    }

    public static final Parcelable.Creator<Language> CREATOR = new Parcelable.Creator<Language>() {
        @Override
        public Language createFromParcel(Parcel in) {
            return new Language(in);
        }

        @Override
        public Language[] newArray(int size) {
            return new Language[size];
        }
    };
}
