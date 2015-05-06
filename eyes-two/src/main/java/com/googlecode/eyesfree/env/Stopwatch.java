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

package com.googlecode.eyesfree.env;

import android.os.SystemClock;

/**
 * An interval timing class. This class does not count time while the device
 * is in deep sleep.
 *
 * @author Sharvil Nanavati
 */
public class Stopwatch {
  private final String name;
  private long startTime;
  private long totalTime;
  private boolean isRunning;

  public Stopwatch() {
    name = null;
  }

  public Stopwatch(String name) {
    this.name = name;
  }

  public void start() {
    if (isRunning) {
      return;
    }

    startTime = SystemClock.uptimeMillis();
    isRunning = true;
  }

  public void stop() {
    if (!isRunning) {
      return;
    }

    long now = SystemClock.uptimeMillis();
    totalTime += (now - startTime);
    isRunning = false;
  }

  public void reset() {
    startTime = SystemClock.uptimeMillis();
    totalTime = 0;
  }

  public boolean isRunning() {
    return isRunning;
  }

  public long getElapsedMilliseconds() {
    if (isRunning) {
      long now = SystemClock.uptimeMillis();
      totalTime += (now - startTime);
      startTime = now;
    }
    return totalTime;
  }

  @Override
  public String toString() {
    if (name != null) {
      return "[" + name + ": " + totalTime + "ms]";
    }

    return totalTime + "ms";
  }
}
