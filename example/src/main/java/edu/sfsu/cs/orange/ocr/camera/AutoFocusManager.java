/*
 * Copyright (C) 2012 ZXing authors
 * Copyright 2012 Robert Theis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package edu.sfsu.cs.orange.ocr.camera;

import android.content.Context;
import android.content.SharedPreferences;
import android.hardware.Camera;
import android.preference.PreferenceManager;
import android.util.Log;
import edu.sfsu.cs.orange.ocr.PreferencesActivity;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Timer;
import java.util.TimerTask;

public final class AutoFocusManager implements Camera.AutoFocusCallback {

  private static final String TAG = AutoFocusManager.class.getSimpleName();

  private static final long AUTO_FOCUS_INTERVAL_MS = 3500L;
  private static final Collection<String> FOCUS_MODES_CALLING_AF;
  static {
    FOCUS_MODES_CALLING_AF = new ArrayList<String>(2);
    FOCUS_MODES_CALLING_AF.add(Camera.Parameters.FOCUS_MODE_AUTO);
    FOCUS_MODES_CALLING_AF.add(Camera.Parameters.FOCUS_MODE_MACRO);
  }

  private boolean active;
  private boolean manual;
  private final boolean useAutoFocus;
  private final Camera camera;
  private final Timer timer;
  private TimerTask outstandingTask;

  AutoFocusManager(Context context, Camera camera) {
    this.camera = camera;
    timer = new Timer(true);
    SharedPreferences sharedPrefs = PreferenceManager.getDefaultSharedPreferences(context);
    String currentFocusMode = camera.getParameters().getFocusMode();
    useAutoFocus =
        sharedPrefs.getBoolean(PreferencesActivity.KEY_AUTO_FOCUS, true) &&
        FOCUS_MODES_CALLING_AF.contains(currentFocusMode);
    Log.i(TAG, "Current focus mode '" + currentFocusMode + "'; use auto focus? " + useAutoFocus);
    manual = false;
    checkAndStart();
  }

  @Override
  public synchronized void onAutoFocus(boolean success, Camera theCamera) {
    if (active && !manual) {
      outstandingTask = new TimerTask() {
        @Override
        public void run() {
          checkAndStart();
        }
      };
      timer.schedule(outstandingTask, AUTO_FOCUS_INTERVAL_MS);
    }
    manual = false;
  }

  void checkAndStart() {
  	if (useAutoFocus) {
  	  active = true;
      start();
    }
  }

  synchronized void start() {
	  try {
		  camera.autoFocus(this);
	  } catch (RuntimeException re) {
		  // Have heard RuntimeException reported in Android 4.0.x+; continue?
		  Log.w(TAG, "Unexpected exception while focusing", re);
	  }
  }

  /**
   * Performs a manual auto-focus after the given delay.
   * @param delay Time to wait before auto-focusing, in milliseconds
   */
  synchronized void start(long delay) {
  	outstandingTask = new TimerTask() {
  		@Override
  		public void run() {
  			manual = true;
  			start();
  		}
  	};
  	timer.schedule(outstandingTask, delay);
  }

  synchronized void stop() {
    if (useAutoFocus) {
      camera.cancelAutoFocus();
    }
    if (outstandingTask != null) {
      outstandingTask.cancel();
      outstandingTask = null;
    }
    active = false;
    manual = false;
  }

}
