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
import android.os.AsyncTask;
import android.util.Log;

import com.googlecode.eyesfree.ocr.R;
import com.googlecode.eyesfree.ocr.client.Language;
import com.googlecode.eyesfree.ocr.intent.LanguagesActivity.LanguageData;

import org.w3c.dom.Document;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.StringReader;
import java.net.HttpURLConnection;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;
import java.util.TreeSet;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

/**
 * @author alanv@google.com (Alan Viverette)
 */
class XmlLoader extends AsyncTask<String, Integer, TreeSet<LanguageData>> {
    private static final String TAG = "XmlLoader";

    private static final String TYPE_LANGUAGE = "language";

    private static final int VERSION = 3;

    private final List<Language> mAvailable;

    private String mCachedXml;

    private ProgressDialog mDialog;

    public XmlLoader(Activity activity, List<Language> available) {
        mAvailable = available;
        mCachedXml = null;

        String message = activity.getString(R.string.manage_loading);

        mDialog = new ProgressDialog(activity);
        mDialog.setMax(100);
        mDialog.setProgress(0);
        mDialog.setIndeterminate(false);
        mDialog.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
        mDialog.setMessage(message);
        mDialog.setCancelable(true);
        mDialog.setOwnerActivity(activity);
    }

    public XmlLoader setCachedXml(String cachedXml) {
        mCachedXml = cachedXml;

        return this;
    }

    protected String getCachedXml() {
        return mCachedXml;
    }

    @Override
    protected void onPreExecute() {
        mDialog.show();
    }

    @Override
    protected void onProgressUpdate(Integer... progress) {
        mDialog.setProgress(progress[0]);
    }

    @Override
    protected void onPostExecute(TreeSet<LanguageData> result) {
        mDialog.dismiss();
    }

    @Override
    protected TreeSet<LanguageData> doInBackground(String... urls) {
        Document xmldoc = null;

        if (mCachedXml != null) {
            xmldoc = loadFromCachedXML();
        } else {
            xmldoc = loadFromURL(urls[0]);
        }

        if (xmldoc == null) {
            return null;
        }

        Node languages = xmldoc.getFirstChild();

        if (languages == null || !languages.getNodeName().equals("languages")) {
            Log.e(TAG, "Missing languages node");
            return null;
        }

        NamedNodeMap attr = languages.getAttributes();
        Node node = attr.getNamedItem("version");

        // Check version of remote languages list
        int version = Integer.parseInt(node.getNodeValue());
        if (version != VERSION) {
            Log.e(TAG, "Incorrect version (is " + VERSION + ", should be " + version);
            return null;
        }

        // Load the ISO 639-2 name of each language into a set
        TreeSet<String> available6392 = new TreeSet<String>();

        for (Language lang : mAvailable) {
            available6392.add(lang.iso_639_2);
        }

        // Traverse the XML document and create LanguageData objects
        final TreeSet<LanguageData> displayLanguages = new TreeSet<LanguageData>();
        final NodeList nodes = languages.getChildNodes();
        final int count = nodes.getLength();

        for (int i = 0; i < count; i++) {
            publishProgress(100 * i / count);

            Node language = nodes.item(i);

            if (TYPE_LANGUAGE.equals(language.getNodeName())) {
                LanguageData data = inflateLanguage(language, available6392);

                // Display the language if it is installed or visible
                if (data.installed || !data.hidden) {
                    displayLanguages.add(data);
                }
            }
        }

        return displayLanguages;
    }

    private Document loadFromCachedXML() {
        Document xmldoc = null;

        try {
            InputSource source = new InputSource(new StringReader(mCachedXml));
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            xmldoc = builder.parse(source);

        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
        } catch (SAXException e) {
            e.printStackTrace();
        }

        mCachedXml = null;

        return xmldoc;
    }

    private Document loadFromURL(String url) {
        Document xmldoc = null;

        try {
            URL xmlUrl = new URL(url);
            DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
            DocumentBuilder builder = factory.newDocumentBuilder();
            HttpURLConnection http = (HttpURLConnection) xmlUrl.openConnection();
            xmldoc = builder.parse(http.getInputStream());

            DOMSource xmlSource = new DOMSource(xmldoc);
            ByteArrayOutputStream output = new ByteArrayOutputStream();
            StreamResult outputTarget = new StreamResult(output);

            try {
                Transformer transformer = TransformerFactory.newInstance().newTransformer();
                transformer.transform(xmlSource, outputTarget);
                mCachedXml = output.toString();
            } catch (TransformerException e) {
                e.printStackTrace();
            }
        } catch (MalformedURLException e) {
            e.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (ParserConfigurationException e) {
            e.printStackTrace();
        } catch (SAXException e) {
            e.printStackTrace();
        }

        return xmldoc;
    }

    private LanguageData inflateLanguage(Node language, TreeSet<String> available6392) {
        LanguageData data = new LanguageData();
        NamedNodeMap attr = language.getAttributes();
        Node node;

        node = attr.getNamedItem("size");
        data.size = node.getNodeValue();
        node = attr.getNamedItem("file");
        data.file = node.getNodeValue();
        node = attr.getNamedItem("iso6392");
        data.iso6392 = node.getNodeValue();
        node = attr.getNamedItem("name");
        data.name = node.getNodeValue();
        node = attr.getNamedItem("hidden");
        data.hidden = (node != null);

        data.installed = available6392.contains(data.iso6392);

        return data;
    }
}
