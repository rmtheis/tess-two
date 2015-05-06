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

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.Intent;
import android.net.Uri;

/**
 * Creates an alert dialog that directs the user to the App Market.
 *
 * @author alanv@google.com (Alan Viverette)
 */
public class VersionAlert {
    private final static String MARKET_URI = "market://search?q=pname:com.googlecode.eyesfree.ocr";

    public static String install = "This application requires the Mobile OCR library for text recognition.";
    public static String install_title = "Install OCR Library";
    public static String install_positive = "Install";
    public static String install_negative = "Do not install";
    public static String update = "This application requires a newer version of the Mobile OCR library.";
    public static String update_title = "Update OCR Library";
    public static String update_positive = "Update";
    public static String update_negative = "Do not update";
    public static String sdcard = "Please insert an SD card or turn off USB storage.";
    public static String sdcard_title = "Insert SD Card";
    public static String sdcard_neutral = "OK";
    public static String languages = "Please install at least one Mobile OCR language pack.";
    public static String languages_title = "Install OCR Languages";
    public static String languages_positive = "Select language";
    public static String languages_negative = "Do not install";

    private VersionAlert() {
        // This class is not instantiable.
    }

    /**
     * Returns an install dialog.
     *
     * @param context
     * @param onNegative
     * @return An install dialog.
     */
    public static AlertDialog createInstallAlert(final Context context, OnClickListener onNegative) {
        OnClickListener onPositive = new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Uri marketUri = Uri.parse(MARKET_URI);
                Intent marketIntent = new Intent(Intent.ACTION_VIEW, marketUri);
                context.startActivity(marketIntent);
            }
        };

        AlertDialog alert = new AlertDialog.Builder(context).setMessage(install)
                .setTitle(install_title).setPositiveButton(install_positive, onPositive)
                .setNegativeButton(install_negative, onNegative).create();

        return alert;
    }

    /**
     * Returns an update dialog.
     *
     * @param context
     * @param onNegative
     * @return An update dialog.
     */
    public static AlertDialog createUpdateAlert(final Context context, OnClickListener onNegative) {
        OnClickListener onPositive = new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Uri marketUri = Uri.parse(MARKET_URI);
                Intent marketIntent = new Intent(Intent.ACTION_VIEW, marketUri);
                context.startActivity(marketIntent);
            }
        };

        AlertDialog alert = new AlertDialog.Builder(context).setMessage(update)
                .setTitle(update_title).setPositiveButton(update_positive, onPositive)
                .setNegativeButton(update_negative, onNegative).create();

        return alert;
    }

    /**
     * Returns a storage alert dialog.
     *
     * @param context
     * @param onNeutral
     * @return A storage alert dialog.
     */
    public static AlertDialog createStorageAlert(final Context context, OnClickListener onNeutral) {
        AlertDialog alert = new AlertDialog.Builder(context).setMessage(sdcard)
                .setTitle(sdcard_title).setNeutralButton(sdcard_neutral, onNeutral).create();

        return alert;
    }

    /**
     * Creates an alert requesting that the user install a language.
     *
     * @param context
     * @param onPositive action to perform if the user accepts
     * @param onNegative action to perform if the user declines
     * @return alert dialog
     */
    public static AlertDialog createLanguagesAlert(final Context context,
            final OnClickListener onPositive, OnClickListener onNegative) {
        OnClickListener onRealPositive = new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int which) {
                Intent languagesIntent = new Intent(Intents.Languages.ACTION);
                context.startActivity(languagesIntent);
                onPositive.onClick(dialog, which);
            }
        };

        AlertDialog alert = new AlertDialog.Builder(context).setMessage(languages)
                .setTitle(languages_title).setPositiveButton(languages_positive, onRealPositive)
                .setNegativeButton(languages_negative, onNegative).create();

        return alert;
    }
}
