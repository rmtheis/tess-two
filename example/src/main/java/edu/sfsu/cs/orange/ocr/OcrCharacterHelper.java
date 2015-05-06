/*
 * Copyright 2011 Robert Theis
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
package edu.sfsu.cs.orange.ocr;

import android.content.SharedPreferences;

/**
 * Helper class to enable language-specific character blacklists/whitelists.
 */
public class OcrCharacterHelper {
  public static final String KEY_CHARACTER_BLACKLIST_AFRIKAANS = "preference_character_blacklist_afrikaans";
  public static final String KEY_CHARACTER_BLACKLIST_ALBANIAN = "preference_character_blacklist_albanian";
  public static final String KEY_CHARACTER_BLACKLIST_ARABIC = "preference_character_blacklist_arabic";
  public static final String KEY_CHARACTER_BLACKLIST_AZERI = "preference_character_blacklist_azeri";
  public static final String KEY_CHARACTER_BLACKLIST_BASQUE = "preference_character_blacklist_basque";
  public static final String KEY_CHARACTER_BLACKLIST_BELARUSIAN = "preference_character_blacklist_belarusian";
  public static final String KEY_CHARACTER_BLACKLIST_BENGALI = "preference_character_blacklist_bengali";
  public static final String KEY_CHARACTER_BLACKLIST_BULGARIAN = "preference_character_blacklist_bulgarian";
  public static final String KEY_CHARACTER_BLACKLIST_CATALAN = "preference_character_blacklist_catalan";
  public static final String KEY_CHARACTER_BLACKLIST_CHINESE_SIMPLIFIED = "preference_character_blacklist_chinese_simplified";
  public static final String KEY_CHARACTER_BLACKLIST_CHINESE_TRADITIONAL = "preference_character_blacklist_chinese_traditional";
  public static final String KEY_CHARACTER_BLACKLIST_CROATIAN = "preference_character_blacklist_croatian";
  public static final String KEY_CHARACTER_BLACKLIST_CZECH = "preference_character_blacklist_czech";
  public static final String KEY_CHARACTER_BLACKLIST_DANISH = "preference_character_blacklist_danish";
  public static final String KEY_CHARACTER_BLACKLIST_DUTCH = "preference_character_blacklist_dutch";
  public static final String KEY_CHARACTER_BLACKLIST_ENGLISH = "preference_character_blacklist_english";
  public static final String KEY_CHARACTER_BLACKLIST_ESTONIAN = "preference_character_blacklist_estonian";
  public static final String KEY_CHARACTER_BLACKLIST_FINNISH = "preference_character_blacklist_finnish";
  public static final String KEY_CHARACTER_BLACKLIST_FRENCH = "preference_character_blacklist_french";
  public static final String KEY_CHARACTER_BLACKLIST_GALICIAN = "preference_character_blacklist_galician";
  public static final String KEY_CHARACTER_BLACKLIST_GERMAN = "preference_character_blacklist_german";
  public static final String KEY_CHARACTER_BLACKLIST_GREEK = "preference_character_blacklist_greek";
  public static final String KEY_CHARACTER_BLACKLIST_HEBREW = "preference_character_blacklist_hebrew";
  public static final String KEY_CHARACTER_BLACKLIST_HINDI = "preference_character_blacklist_hindi";
  public static final String KEY_CHARACTER_BLACKLIST_HUNGARIAN = "preference_character_blacklist_hungarian";
  public static final String KEY_CHARACTER_BLACKLIST_ICELANDIC = "preference_character_blacklist_icelandic";
  public static final String KEY_CHARACTER_BLACKLIST_INDONESIAN = "preference_character_blacklist_indonesian";
  public static final String KEY_CHARACTER_BLACKLIST_ITALIAN = "preference_character_blacklist_italian";
  public static final String KEY_CHARACTER_BLACKLIST_JAPANESE = "preference_character_blacklist_japanese";
  public static final String KEY_CHARACTER_BLACKLIST_KANNADA = "preference_character_blacklist_kannada";
  public static final String KEY_CHARACTER_BLACKLIST_KOREAN = "preference_character_blacklist_korean";
  public static final String KEY_CHARACTER_BLACKLIST_LATVIAN = "preference_character_blacklist_latvian";
  public static final String KEY_CHARACTER_BLACKLIST_LITHUANIAN = "preference_character_blacklist_lithuanian";
  public static final String KEY_CHARACTER_BLACKLIST_MACEDONIAN = "preference_character_blacklist_macedonian";
  public static final String KEY_CHARACTER_BLACKLIST_MALAY = "preference_character_blacklist_malay";
  public static final String KEY_CHARACTER_BLACKLIST_MALAYALAM = "preference_character_blacklist_malayalam";
  public static final String KEY_CHARACTER_BLACKLIST_MALTESE = "preference_character_blacklist_maltese";
  public static final String KEY_CHARACTER_BLACKLIST_NORWEGIAN = "preference_character_blacklist_norwegian";
  public static final String KEY_CHARACTER_BLACKLIST_POLISH = "preference_character_blacklist_polish";
  public static final String KEY_CHARACTER_BLACKLIST_PORTUGUESE = "preference_character_blacklist_portuguese";
  public static final String KEY_CHARACTER_BLACKLIST_ROMANIAN = "preference_character_blacklist_romanian";
  public static final String KEY_CHARACTER_BLACKLIST_RUSSIAN = "preference_character_blacklist_russian";
  public static final String KEY_CHARACTER_BLACKLIST_SERBIAN = "preference_character_blacklist_serbian";
  public static final String KEY_CHARACTER_BLACKLIST_SLOVAK = "preference_character_blacklist_slovak";
  public static final String KEY_CHARACTER_BLACKLIST_SLOVENIAN = "preference_character_blacklist_slovenian";
  public static final String KEY_CHARACTER_BLACKLIST_SPANISH = "preference_character_blacklist_spanish";
  public static final String KEY_CHARACTER_BLACKLIST_SWAHILI = "preference_character_blacklist_swahili";
  public static final String KEY_CHARACTER_BLACKLIST_SWEDISH = "preference_character_blacklist_swedish";
  public static final String KEY_CHARACTER_BLACKLIST_TAGALOG = "preference_character_blacklist_tagalog";
  public static final String KEY_CHARACTER_BLACKLIST_TAMIL = "preference_character_blacklist_tamil";
  public static final String KEY_CHARACTER_BLACKLIST_TELUGU = "preference_character_blacklist_telugu";
  public static final String KEY_CHARACTER_BLACKLIST_THAI = "preference_character_blacklist_thai";
  public static final String KEY_CHARACTER_BLACKLIST_TURKISH = "preference_character_blacklist_turkish";
  public static final String KEY_CHARACTER_BLACKLIST_UKRAINIAN = "preference_character_blacklist_ukrainian";
  public static final String KEY_CHARACTER_BLACKLIST_VIETNAMESE = "preference_character_blacklist_vietnamese";

  public static final String KEY_CHARACTER_WHITELIST_AFRIKAANS = "preference_character_whitelist_afrikaans";
  public static final String KEY_CHARACTER_WHITELIST_ALBANIAN = "preference_character_whitelist_albanian";
  public static final String KEY_CHARACTER_WHITELIST_ARABIC = "preference_character_whitelist_arabic";
  public static final String KEY_CHARACTER_WHITELIST_AZERI = "preference_character_whitelist_azeri";
  public static final String KEY_CHARACTER_WHITELIST_BASQUE = "preference_character_whitelist_basque";
  public static final String KEY_CHARACTER_WHITELIST_BELARUSIAN = "preference_character_whitelist_belarusian";
  public static final String KEY_CHARACTER_WHITELIST_BENGALI = "preference_character_whitelist_bengali";
  public static final String KEY_CHARACTER_WHITELIST_BULGARIAN = "preference_character_whitelist_bulgarian";
  public static final String KEY_CHARACTER_WHITELIST_CATALAN = "preference_character_whitelist_catalan";
  public static final String KEY_CHARACTER_WHITELIST_CHINESE_SIMPLIFIED = "preference_character_whitelist_chinese_simplified";
  public static final String KEY_CHARACTER_WHITELIST_CHINESE_TRADITIONAL = "preference_character_whitelist_chinese_traditional";
  public static final String KEY_CHARACTER_WHITELIST_CROATIAN = "preference_character_whitelist_croatian";
  public static final String KEY_CHARACTER_WHITELIST_CZECH = "preference_character_whitelist_czech";
  public static final String KEY_CHARACTER_WHITELIST_DANISH = "preference_character_whitelist_danish";
  public static final String KEY_CHARACTER_WHITELIST_DUTCH = "preference_character_whitelist_dutch";
  public static final String KEY_CHARACTER_WHITELIST_ENGLISH = "preference_character_whitelist_english";
  public static final String KEY_CHARACTER_WHITELIST_ESTONIAN = "preference_character_whitelist_estonian";
  public static final String KEY_CHARACTER_WHITELIST_FINNISH = "preference_character_whitelist_finnish";
  public static final String KEY_CHARACTER_WHITELIST_FRENCH = "preference_character_whitelist_french";
  public static final String KEY_CHARACTER_WHITELIST_GALICIAN = "preference_character_whitelist_galician";
  public static final String KEY_CHARACTER_WHITELIST_GERMAN = "preference_character_whitelist_german";
  public static final String KEY_CHARACTER_WHITELIST_GREEK = "preference_character_whitelist_greek";
  public static final String KEY_CHARACTER_WHITELIST_HEBREW = "preference_character_whitelist_hebrew";
  public static final String KEY_CHARACTER_WHITELIST_HINDI = "preference_character_whitelist_hindi";
  public static final String KEY_CHARACTER_WHITELIST_HUNGARIAN = "preference_character_whitelist_hungarian";
  public static final String KEY_CHARACTER_WHITELIST_ICELANDIC = "preference_character_whitelist_icelandic";
  public static final String KEY_CHARACTER_WHITELIST_INDONESIAN = "preference_character_whitelist_indonesian";
  public static final String KEY_CHARACTER_WHITELIST_ITALIAN = "preference_character_whitelist_italian";
  public static final String KEY_CHARACTER_WHITELIST_JAPANESE = "preference_character_whitelist_japanese";
  public static final String KEY_CHARACTER_WHITELIST_KANNADA = "preference_character_whitelist_kannada";
  public static final String KEY_CHARACTER_WHITELIST_KOREAN = "preference_character_whitelist_korean";
  public static final String KEY_CHARACTER_WHITELIST_LATVIAN = "preference_character_whitelist_latvian";
  public static final String KEY_CHARACTER_WHITELIST_LITHUANIAN = "preference_character_whitelist_lithuanian";
  public static final String KEY_CHARACTER_WHITELIST_MACEDONIAN = "preference_character_whitelist_macedonian";
  public static final String KEY_CHARACTER_WHITELIST_MALAY = "preference_character_whitelist_malay";
  public static final String KEY_CHARACTER_WHITELIST_MALAYALAM = "preference_character_whitelist_malayalam";
  public static final String KEY_CHARACTER_WHITELIST_MALTESE = "preference_character_whitelist_maltese";
  public static final String KEY_CHARACTER_WHITELIST_NORWEGIAN = "preference_character_whitelist_norwegian";
  public static final String KEY_CHARACTER_WHITELIST_POLISH = "preference_character_whitelist_polish";
  public static final String KEY_CHARACTER_WHITELIST_PORTUGUESE = "preference_character_whitelist_portuguese";
  public static final String KEY_CHARACTER_WHITELIST_ROMANIAN = "preference_character_whitelist_romanian";
  public static final String KEY_CHARACTER_WHITELIST_RUSSIAN = "preference_character_whitelist_russian";
  public static final String KEY_CHARACTER_WHITELIST_SERBIAN = "preference_character_whitelist_serbian";
  public static final String KEY_CHARACTER_WHITELIST_SLOVAK = "preference_character_whitelist_slovak";
  public static final String KEY_CHARACTER_WHITELIST_SLOVENIAN = "preference_character_whitelist_slovenian";
  public static final String KEY_CHARACTER_WHITELIST_SPANISH = "preference_character_whitelist_spanish";
  public static final String KEY_CHARACTER_WHITELIST_SWAHILI = "preference_character_whitelist_swahili";
  public static final String KEY_CHARACTER_WHITELIST_SWEDISH = "preference_character_whitelist_swedish";
  public static final String KEY_CHARACTER_WHITELIST_TAGALOG = "preference_character_whitelist_tagalog";
  public static final String KEY_CHARACTER_WHITELIST_TAMIL = "preference_character_whitelist_tamil";
  public static final String KEY_CHARACTER_WHITELIST_TELUGU = "preference_character_whitelist_telugu";
  public static final String KEY_CHARACTER_WHITELIST_THAI = "preference_character_whitelist_thai";
  public static final String KEY_CHARACTER_WHITELIST_TURKISH = "preference_character_whitelist_turkish";
  public static final String KEY_CHARACTER_WHITELIST_UKRAINIAN = "preference_character_whitelist_ukrainian";
  public static final String KEY_CHARACTER_WHITELIST_VIETNAMESE = "preference_character_whitelist_vietnamese";  
  
  private OcrCharacterHelper() {} // Private constructor to enforce noninstantiability
  
  public static String getDefaultBlacklist(String languageCode) {
    //final String DEFAULT_BLACKLIST = "`~|";
    
    if (languageCode.equals("afr")) { return ""; } // Afrikaans
    else if (languageCode.equals("sqi")) { return ""; } // Albanian
    else if (languageCode.equals("ara")) { return ""; } // Arabic
    else if (languageCode.equals("aze")) { return ""; } // Azeri
    else if (languageCode.equals("eus")) { return ""; } // Basque
    else if (languageCode.equals("bel")) { return ""; } // Belarusian
    else if (languageCode.equals("ben")) { return ""; } // Bengali
    else if (languageCode.equals("bul")) { return ""; } // Bulgarian
    else if (languageCode.equals("cat")) { return ""; } // Catalan
    else if (languageCode.equals("chi_sim")) { return ""; } // Chinese (Simplified)
    else if (languageCode.equals("chi_tra")) { return ""; } // Chinese (Traditional)
    else if (languageCode.equals("hrv")) { return ""; } // Croatian
    else if (languageCode.equals("ces")) { return ""; } // Czech
    else if (languageCode.equals("dan")) { return ""; } // Danish
    else if (languageCode.equals("nld")) { return ""; } // Dutch
    else if (languageCode.equals("eng")) { return ""; } // English
    else if (languageCode.equals("est")) { return ""; } // Estonian
    else if (languageCode.equals("fin")) { return ""; } // Finnish
    else if (languageCode.equals("fra")) { return ""; } // French
    else if (languageCode.equals("glg")) { return ""; } // Galician
    else if (languageCode.equals("deu")) { return ""; } // German
    else if (languageCode.equals("ell")) { return ""; } // Greek
    else if (languageCode.equals("heb")) { return ""; } // Hebrew
    else if (languageCode.equals("hin")) { return ""; } // Hindi
    else if (languageCode.equals("hun")) { return ""; } // Hungarian
    else if (languageCode.equals("isl")) { return ""; } // Icelandic
    else if (languageCode.equals("ind")) { return ""; } // Indonesian
    else if (languageCode.equals("ita")) { return ""; } // Italian
    else if (languageCode.equals("jpn")) { return ""; } // Japanese
    else if (languageCode.equals("kan")) { return ""; } // Kannada
    else if (languageCode.equals("kor")) { return ""; } // Korean
    else if (languageCode.equals("lav")) { return ""; } // Latvian
    else if (languageCode.equals("lit")) { return ""; } // Lithuanian
    else if (languageCode.equals("mkd")) { return ""; } // Macedonian
    else if (languageCode.equals("msa")) { return ""; } // Malay
    else if (languageCode.equals("mal")) { return ""; } // Malayalam
    else if (languageCode.equals("mlt")) { return ""; } // Maltese
    else if (languageCode.equals("nor")) { return ""; } // Norwegian
    else if (languageCode.equals("pol")) { return ""; } // Polish
    else if (languageCode.equals("por")) { return ""; } // Portuguese
    else if (languageCode.equals("ron")) { return ""; } // Romanian
    else if (languageCode.equals("rus")) { return ""; } // Russian
    else if (languageCode.equals("srp")) { return ""; } // Serbian (Latin)
    else if (languageCode.equals("slk")) { return ""; } // Slovak
    else if (languageCode.equals("slv")) { return ""; } // Slovenian
    else if (languageCode.equals("spa")) { return ""; } // Spanish
    else if (languageCode.equals("swa")) { return ""; } // Swahili
    else if (languageCode.equals("swe")) { return ""; } // Swedish
    else if (languageCode.equals("tgl")) { return ""; } // Tagalog
    else if (languageCode.equals("tam")) { return ""; } // Tamil
    else if (languageCode.equals("tel")) { return ""; } // Telugu
    else if (languageCode.equals("tha")) { return ""; } // Thai
    else if (languageCode.equals("tur")) { return ""; } // Turkish
    else if (languageCode.equals("ukr")) { return ""; } // Ukrainian
    else if (languageCode.equals("vie")) { return ""; } // Vietnamese
    else {
      throw new IllegalArgumentException();
    }
  }
  
  public static String getDefaultWhitelist(String languageCode) {
    if (languageCode.equals("afr")) { return ""; } // Afrikaans
    else if (languageCode.equals("sqi")) { return ""; } // Albanian
    else if (languageCode.equals("ara")) { return ""; } // Arabic
    else if (languageCode.equals("aze")) { return ""; } // Azeri
    else if (languageCode.equals("eus")) { return ""; } // Basque
    else if (languageCode.equals("bel")) { return ""; } // Belarusian
    else if (languageCode.equals("ben")) { return ""; } // Bengali
    else if (languageCode.equals("bul")) { return ""; } // Bulgarian
    else if (languageCode.equals("cat")) { return ""; } // Catalan
    else if (languageCode.equals("chi_sim")) { return ""; } // Chinese (Simplified)
    else if (languageCode.equals("chi_tra")) { return ""; } // Chinese (Traditional)
    else if (languageCode.equals("hrv")) { return ""; } // Croatian
    else if (languageCode.equals("ces")) { return ""; } // Czech
    else if (languageCode.equals("dan")) { return ""; } // Danish
    else if (languageCode.equals("nld")) { return ""; } // Dutch
    else if (languageCode.equals("eng")) { return "!?@#$%&*()<>_-+=/.,:;'\"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"; } // English
    else if (languageCode.equals("est")) { return ""; } // Estonian
    else if (languageCode.equals("fin")) { return ""; } // Finnish
    else if (languageCode.equals("fra")) { return ""; } // French
    else if (languageCode.equals("glg")) { return ""; } // Galician
    else if (languageCode.equals("deu")) { return ""; } // German
    else if (languageCode.equals("ell")) { return ""; } // Greek
    else if (languageCode.equals("heb")) { return ""; } // Hebrew
    else if (languageCode.equals("hin")) { return ""; } // Hindi
    else if (languageCode.equals("hun")) { return ""; } // Hungarian
    else if (languageCode.equals("isl")) { return ""; } // Icelandic
    else if (languageCode.equals("ind")) { return ""; } // Indonesian
    else if (languageCode.equals("ita")) { return ""; } // Italian
    else if (languageCode.equals("jpn")) { return ""; } // Japanese
    else if (languageCode.equals("kan")) { return ""; } // Kannada
    else if (languageCode.equals("kor")) { return ""; } // Korean
    else if (languageCode.equals("lav")) { return ""; } // Latvian
    else if (languageCode.equals("lit")) { return ""; } // Lithuanian
    else if (languageCode.equals("mkd")) { return ""; } // Macedonian
    else if (languageCode.equals("msa")) { return ""; } // Malay
    else if (languageCode.equals("mal")) { return ""; } // Malayalam
    else if (languageCode.equals("mlt")) { return ""; } // Maltese
    else if (languageCode.equals("nor")) { return ""; } // Norwegian
    else if (languageCode.equals("pol")) { return ""; } // Polish
    else if (languageCode.equals("por")) { return ""; } // Portuguese
    else if (languageCode.equals("ron")) { return ""; } // Romanian
    else if (languageCode.equals("rus")) { return ""; } // Russian
    else if (languageCode.equals("srp")) { return ""; } // Serbian (Latin)
    else if (languageCode.equals("slk")) { return ""; } // Slovak
    else if (languageCode.equals("slv")) { return ""; } // Slovenian
    else if (languageCode.equals("spa")) { return ""; } // Spanish
    else if (languageCode.equals("swa")) { return ""; } // Swahili
    else if (languageCode.equals("swe")) { return ""; } // Swedish
    else if (languageCode.equals("tgl")) { return ""; } // Tagalog
    else if (languageCode.equals("tam")) { return ""; } // Tamil
    else if (languageCode.equals("tel")) { return ""; } // Telugu
    else if (languageCode.equals("tha")) { return ""; } // Thai
    else if (languageCode.equals("tur")) { return ""; } // Turkish
    else if (languageCode.equals("ukr")) { return ""; } // Ukrainian
    else if (languageCode.equals("vie")) { return ""; } // Vietnamese
    else {
      throw new IllegalArgumentException();
    }
  }

  public static String getBlacklist(SharedPreferences prefs, String languageCode) {
    if (languageCode.equals("afr")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_AFRIKAANS, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("sqi")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ALBANIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ara")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ARABIC, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("aze")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_AZERI, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("eus")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_BASQUE, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("bel")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_BELARUSIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ben")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_BENGALI, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("bul")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_BULGARIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("cat")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_CATALAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("chi_sim")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_CHINESE_SIMPLIFIED, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("chi_tra")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_CHINESE_TRADITIONAL, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("hrv")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_CROATIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ces")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_CZECH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("dan")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_DANISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("nld")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_DUTCH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("eng")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ENGLISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("est")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ESTONIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("fin")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_FINNISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("fra")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_FRENCH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("glg")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_GALICIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("deu")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_GERMAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ell")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_GREEK, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("heb")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_HEBREW, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("hin")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_HINDI, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("hun")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_HUNGARIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("isl")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ICELANDIC, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ind")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_INDONESIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ita")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ITALIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("jpn")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_JAPANESE, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("kan")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_KANNADA, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("kor")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_KOREAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("lav")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_LATVIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("lit")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_LITHUANIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("mkd")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_MACEDONIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("msa")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_MALAY, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("mal")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_MALAYALAM, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("mlt")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_MALTESE, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("nor")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_NORWEGIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("pol")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_POLISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("por")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_PORTUGUESE, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ron")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_ROMANIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("rus")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_RUSSIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("srp")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_SERBIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("slk")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_SLOVAK, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("slv")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_SLOVENIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("spa")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_SPANISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("swa")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_SWAHILI, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("swe")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_SWEDISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("tgl")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_TAGALOG, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("tam")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_TAMIL, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("tel")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_TELUGU, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("tha")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_THAI, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("tur")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_TURKISH, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("ukr")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_UKRAINIAN, getDefaultBlacklist(languageCode)); }
    else if (languageCode.equals("vie")) { return prefs.getString(KEY_CHARACTER_BLACKLIST_VIETNAMESE, getDefaultBlacklist(languageCode)); }
    else {
      throw new IllegalArgumentException();
    }    
  }
  
  public static String getWhitelist(SharedPreferences prefs, String languageCode) {
    if (languageCode.equals("afr")) { return prefs.getString(KEY_CHARACTER_WHITELIST_AFRIKAANS, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("sqi")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ALBANIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ara")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ARABIC, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("aze")) { return prefs.getString(KEY_CHARACTER_WHITELIST_AZERI, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("eus")) { return prefs.getString(KEY_CHARACTER_WHITELIST_BASQUE, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("bel")) { return prefs.getString(KEY_CHARACTER_WHITELIST_BELARUSIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ben")) { return prefs.getString(KEY_CHARACTER_WHITELIST_BENGALI, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("bul")) { return prefs.getString(KEY_CHARACTER_WHITELIST_BULGARIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("cat")) { return prefs.getString(KEY_CHARACTER_WHITELIST_CATALAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("chi_sim")) { return prefs.getString(KEY_CHARACTER_WHITELIST_CHINESE_SIMPLIFIED, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("chi_tra")) { return prefs.getString(KEY_CHARACTER_WHITELIST_CHINESE_TRADITIONAL, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("hrv")) { return prefs.getString(KEY_CHARACTER_WHITELIST_CROATIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ces")) { return prefs.getString(KEY_CHARACTER_WHITELIST_CZECH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("dan")) { return prefs.getString(KEY_CHARACTER_WHITELIST_DANISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("nld")) { return prefs.getString(KEY_CHARACTER_WHITELIST_DUTCH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("eng")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ENGLISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("est")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ESTONIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("fin")) { return prefs.getString(KEY_CHARACTER_WHITELIST_FINNISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("fra")) { return prefs.getString(KEY_CHARACTER_WHITELIST_FRENCH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("glg")) { return prefs.getString(KEY_CHARACTER_WHITELIST_GALICIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("deu")) { return prefs.getString(KEY_CHARACTER_WHITELIST_GERMAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ell")) { return prefs.getString(KEY_CHARACTER_WHITELIST_GREEK, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("heb")) { return prefs.getString(KEY_CHARACTER_WHITELIST_HEBREW, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("hin")) { return prefs.getString(KEY_CHARACTER_WHITELIST_HINDI, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("hun")) { return prefs.getString(KEY_CHARACTER_WHITELIST_HUNGARIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("isl")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ICELANDIC, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ind")) { return prefs.getString(KEY_CHARACTER_WHITELIST_INDONESIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ita")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ITALIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("jpn")) { return prefs.getString(KEY_CHARACTER_WHITELIST_JAPANESE, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("kan")) { return prefs.getString(KEY_CHARACTER_WHITELIST_KANNADA, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("kor")) { return prefs.getString(KEY_CHARACTER_WHITELIST_KOREAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("lav")) { return prefs.getString(KEY_CHARACTER_WHITELIST_LATVIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("lit")) { return prefs.getString(KEY_CHARACTER_WHITELIST_LITHUANIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("mkd")) { return prefs.getString(KEY_CHARACTER_WHITELIST_MACEDONIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("msa")) { return prefs.getString(KEY_CHARACTER_WHITELIST_MALAY, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("mal")) { return prefs.getString(KEY_CHARACTER_WHITELIST_MALAYALAM, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("mlt")) { return prefs.getString(KEY_CHARACTER_WHITELIST_MALTESE, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("nor")) { return prefs.getString(KEY_CHARACTER_WHITELIST_NORWEGIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("pol")) { return prefs.getString(KEY_CHARACTER_WHITELIST_POLISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("por")) { return prefs.getString(KEY_CHARACTER_WHITELIST_PORTUGUESE, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ron")) { return prefs.getString(KEY_CHARACTER_WHITELIST_ROMANIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("rus")) { return prefs.getString(KEY_CHARACTER_WHITELIST_RUSSIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("srp")) { return prefs.getString(KEY_CHARACTER_WHITELIST_SERBIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("slk")) { return prefs.getString(KEY_CHARACTER_WHITELIST_SLOVAK, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("slv")) { return prefs.getString(KEY_CHARACTER_WHITELIST_SLOVENIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("spa")) { return prefs.getString(KEY_CHARACTER_WHITELIST_SPANISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("swa")) { return prefs.getString(KEY_CHARACTER_WHITELIST_SWAHILI, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("swe")) { return prefs.getString(KEY_CHARACTER_WHITELIST_SWEDISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("tgl")) { return prefs.getString(KEY_CHARACTER_WHITELIST_TAGALOG, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("tam")) { return prefs.getString(KEY_CHARACTER_WHITELIST_TAMIL, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("tel")) { return prefs.getString(KEY_CHARACTER_WHITELIST_TELUGU, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("tha")) { return prefs.getString(KEY_CHARACTER_WHITELIST_THAI, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("tur")) { return prefs.getString(KEY_CHARACTER_WHITELIST_TURKISH, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("ukr")) { return prefs.getString(KEY_CHARACTER_WHITELIST_UKRAINIAN, getDefaultWhitelist(languageCode)); }
    else if (languageCode.equals("vie")) { return prefs.getString(KEY_CHARACTER_WHITELIST_VIETNAMESE, getDefaultWhitelist(languageCode)); }
    else {
      throw new IllegalArgumentException();
    }        
  }
  
  public static void setBlacklist(SharedPreferences prefs, String languageCode, String blacklist) {
    if (languageCode.equals("afr")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_AFRIKAANS, blacklist).commit(); }
    else if (languageCode.equals("sqi")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ALBANIAN, blacklist).commit(); }
    else if (languageCode.equals("ara")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ARABIC, blacklist).commit(); }
    else if (languageCode.equals("aze")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_AZERI, blacklist).commit(); }
    else if (languageCode.equals("eus")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_BASQUE, blacklist).commit(); }
    else if (languageCode.equals("bel")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_BELARUSIAN, blacklist).commit(); }
    else if (languageCode.equals("ben")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_BENGALI, blacklist).commit(); }
    else if (languageCode.equals("bul")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_BULGARIAN, blacklist).commit(); }
    else if (languageCode.equals("cat")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_CATALAN, blacklist).commit(); }
    else if (languageCode.equals("chi_sim")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_CHINESE_SIMPLIFIED, blacklist).commit(); }
    else if (languageCode.equals("chi_tra")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_CHINESE_TRADITIONAL, blacklist).commit(); }
    else if (languageCode.equals("hrv")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_CROATIAN, blacklist).commit(); }
    else if (languageCode.equals("ces")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_CZECH, blacklist).commit(); }
    else if (languageCode.equals("dan")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_DANISH, blacklist).commit(); }
    else if (languageCode.equals("nld")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_DUTCH, blacklist).commit(); }
    else if (languageCode.equals("eng")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ENGLISH, blacklist).commit(); }
    else if (languageCode.equals("est")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ESTONIAN, blacklist).commit(); }
    else if (languageCode.equals("fin")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_FINNISH, blacklist).commit(); }
    else if (languageCode.equals("fra")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_FRENCH, blacklist).commit(); }
    else if (languageCode.equals("glg")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_GALICIAN, blacklist).commit(); }
    else if (languageCode.equals("deu")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_GERMAN, blacklist).commit(); }
    else if (languageCode.equals("ell")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_GREEK, blacklist).commit(); }
    else if (languageCode.equals("heb")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_HEBREW, blacklist).commit(); }
    else if (languageCode.equals("hin")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_HINDI, blacklist).commit(); }
    else if (languageCode.equals("hun")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_HUNGARIAN, blacklist).commit(); }
    else if (languageCode.equals("isl")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ICELANDIC, blacklist).commit(); }
    else if (languageCode.equals("ind")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_INDONESIAN, blacklist).commit(); }
    else if (languageCode.equals("ita")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ITALIAN, blacklist).commit(); }
    else if (languageCode.equals("jpn")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_JAPANESE, blacklist).commit(); }
    else if (languageCode.equals("kan")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_KANNADA, blacklist).commit(); }
    else if (languageCode.equals("kor")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_KOREAN, blacklist).commit(); }
    else if (languageCode.equals("lav")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_LATVIAN, blacklist).commit(); }
    else if (languageCode.equals("lit")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_LITHUANIAN, blacklist).commit(); }
    else if (languageCode.equals("mkd")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_MACEDONIAN, blacklist).commit(); }
    else if (languageCode.equals("msa")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_MALAY, blacklist).commit(); }
    else if (languageCode.equals("mal")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_MALAYALAM, blacklist).commit(); }
    else if (languageCode.equals("mlt")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_MALTESE, blacklist).commit(); }
    else if (languageCode.equals("nor")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_NORWEGIAN, blacklist).commit(); }
    else if (languageCode.equals("pol")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_POLISH, blacklist).commit(); }
    else if (languageCode.equals("por")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_PORTUGUESE, blacklist).commit(); }
    else if (languageCode.equals("ron")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_ROMANIAN, blacklist).commit(); }
    else if (languageCode.equals("rus")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_RUSSIAN, blacklist).commit(); }
    else if (languageCode.equals("srp")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_SERBIAN, blacklist).commit(); }
    else if (languageCode.equals("slk")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_SLOVAK, blacklist).commit(); }
    else if (languageCode.equals("slv")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_SLOVENIAN, blacklist).commit(); }
    else if (languageCode.equals("spa")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_SPANISH, blacklist).commit(); }
    else if (languageCode.equals("swa")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_SWAHILI, blacklist).commit(); }
    else if (languageCode.equals("swe")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_SWEDISH, blacklist).commit(); }
    else if (languageCode.equals("tgl")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_TAGALOG, blacklist).commit(); }
    else if (languageCode.equals("tam")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_TAMIL, blacklist).commit(); }
    else if (languageCode.equals("tel")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_TELUGU, blacklist).commit(); }
    else if (languageCode.equals("tha")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_THAI, blacklist).commit(); }
    else if (languageCode.equals("tur")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_TURKISH, blacklist).commit(); }
    else if (languageCode.equals("ukr")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_UKRAINIAN, blacklist).commit(); }
    else if (languageCode.equals("vie")) { prefs.edit().putString(KEY_CHARACTER_BLACKLIST_VIETNAMESE, blacklist).commit(); }
    else {
      throw new IllegalArgumentException();
    }    
  }
  
  public static void setWhitelist(SharedPreferences prefs, String languageCode, String whitelist) {
    if (languageCode.equals("afr")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_AFRIKAANS, whitelist).commit(); }
    else if (languageCode.equals("sqi")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ALBANIAN, whitelist).commit(); }
    else if (languageCode.equals("ara")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ARABIC, whitelist).commit(); }
    else if (languageCode.equals("aze")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_AZERI, whitelist).commit(); }
    else if (languageCode.equals("eus")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_BASQUE, whitelist).commit(); }
    else if (languageCode.equals("bel")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_BELARUSIAN, whitelist).commit(); }
    else if (languageCode.equals("ben")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_BENGALI, whitelist).commit(); }
    else if (languageCode.equals("bul")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_BULGARIAN, whitelist).commit(); }
    else if (languageCode.equals("cat")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_CATALAN, whitelist).commit(); }
    else if (languageCode.equals("chi_sim")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_CHINESE_SIMPLIFIED, whitelist).commit(); }
    else if (languageCode.equals("chi_tra")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_CHINESE_TRADITIONAL, whitelist).commit(); }
    else if (languageCode.equals("hrv")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_CROATIAN, whitelist).commit(); }
    else if (languageCode.equals("ces")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_CZECH, whitelist).commit(); }
    else if (languageCode.equals("dan")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_DANISH, whitelist).commit(); }
    else if (languageCode.equals("nld")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_DUTCH, whitelist).commit(); }
    else if (languageCode.equals("eng")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ENGLISH, whitelist).commit(); }
    else if (languageCode.equals("est")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ESTONIAN, whitelist).commit(); }
    else if (languageCode.equals("fin")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_FINNISH, whitelist).commit(); }
    else if (languageCode.equals("fra")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_FRENCH, whitelist).commit(); }
    else if (languageCode.equals("glg")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_GALICIAN, whitelist).commit(); }
    else if (languageCode.equals("deu")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_GERMAN, whitelist).commit(); }
    else if (languageCode.equals("ell")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_GREEK, whitelist).commit(); }
    else if (languageCode.equals("heb")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_HEBREW, whitelist).commit(); }
    else if (languageCode.equals("hin")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_HINDI, whitelist).commit(); }
    else if (languageCode.equals("hun")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_HUNGARIAN, whitelist).commit(); }
    else if (languageCode.equals("isl")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ICELANDIC, whitelist).commit(); }
    else if (languageCode.equals("ind")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_INDONESIAN, whitelist).commit(); }
    else if (languageCode.equals("ita")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ITALIAN, whitelist).commit(); }
    else if (languageCode.equals("jpn")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_JAPANESE, whitelist).commit(); }
    else if (languageCode.equals("kan")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_KANNADA, whitelist).commit(); }
    else if (languageCode.equals("kor")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_KOREAN, whitelist).commit(); }
    else if (languageCode.equals("lav")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_LATVIAN, whitelist).commit(); }
    else if (languageCode.equals("lit")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_LITHUANIAN, whitelist).commit(); }
    else if (languageCode.equals("mkd")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_MACEDONIAN, whitelist).commit(); }
    else if (languageCode.equals("msa")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_MALAY, whitelist).commit(); }
    else if (languageCode.equals("mal")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_MALAYALAM, whitelist).commit(); }
    else if (languageCode.equals("mlt")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_MALTESE, whitelist).commit(); }
    else if (languageCode.equals("nor")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_NORWEGIAN, whitelist).commit(); }
    else if (languageCode.equals("pol")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_POLISH, whitelist).commit(); }
    else if (languageCode.equals("por")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_PORTUGUESE, whitelist).commit(); }
    else if (languageCode.equals("ron")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_ROMANIAN, whitelist).commit(); }
    else if (languageCode.equals("rus")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_RUSSIAN, whitelist).commit(); }
    else if (languageCode.equals("srp")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_SERBIAN, whitelist).commit(); }
    else if (languageCode.equals("slk")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_SLOVAK, whitelist).commit(); }
    else if (languageCode.equals("slv")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_SLOVENIAN, whitelist).commit(); }
    else if (languageCode.equals("spa")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_SPANISH, whitelist).commit(); }
    else if (languageCode.equals("swa")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_SWAHILI, whitelist).commit(); }
    else if (languageCode.equals("swe")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_SWEDISH, whitelist).commit(); }
    else if (languageCode.equals("tgl")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_TAGALOG, whitelist).commit(); }
    else if (languageCode.equals("tam")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_TAMIL, whitelist).commit(); }
    else if (languageCode.equals("tel")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_TELUGU, whitelist).commit(); }
    else if (languageCode.equals("tha")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_THAI, whitelist).commit(); }
    else if (languageCode.equals("tur")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_TURKISH, whitelist).commit(); }
    else if (languageCode.equals("ukr")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_UKRAINIAN, whitelist).commit(); }
    else if (languageCode.equals("vie")) { prefs.edit().putString(KEY_CHARACTER_WHITELIST_VIETNAMESE, whitelist).commit(); }
    else {
      throw new IllegalArgumentException();
    }    
  }
}
