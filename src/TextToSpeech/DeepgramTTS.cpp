#include "DeepgramTTS.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <cstdio>
#include <iostream>

using json = nlohmann::json;

static size_t WriteToFile(void* ptr, size_t size, size_t nmemb, void* stream) {
    FILE* f = static_cast<FILE*>(stream);
    return std::fwrite(ptr, size, nmemb, f);
}

bool Deepgram_TTS_ToFile(
    const std::string& apiKey,
    const std::string& model,
    const std::string& text,
    const std::string& outFile,
    const std::string& encoding,
    const std::string& bitRate
) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    FILE* f = std::fopen(outFile.c_str(), "wb");
    if (!f) {
        curl_easy_cleanup(curl);
        return false;
    }

    // Deepgram TTS speak endpoint:
    // POST https://api.deepgram.com/v1/speak?model=...&encoding=mp3&bit_rate=...
    std::string url = "https://api.deepgram.com/v1/speak?model=" + model;

    if (!encoding.empty()) url += "&encoding=" + encoding;
    if (!bitRate.empty())  url += "&bit_rate=" + bitRate;

    json payload;
    payload["text"] = text;
    const std::string body = payload.dump();

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, ("Authorization: Token " + apiKey).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());

    // Write raw audio bytes to file
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteToFile);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

    // Optional: keep it simple; you can add timeouts later
    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    std::fclose(f);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode < 200 || httpCode >= 300) {
        std::cerr << "[DeepgramTTS] Request failed. CURL=" << curl_easy_strerror(res)
                  << " HTTP=" << httpCode << "\n";
        return false;
    }

    return true;
}
