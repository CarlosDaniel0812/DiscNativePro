#include "AIService.h"
#include <juce_events/juce_events.h>
#include <thread>
#include <cstdio>

#if JUCE_WINDOWS
#define popen _popen
#define pclose _pclose
#endif

juce::File AIService::getSettingsFile()
{
    auto appData = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                       .getChildFile("DiscNativePro");
    if (!appData.exists())
        appData.createDirectory();
    return appData.getChildFile("settings.json");
}

AIProvider AIService::getProvider()
{
    auto file = getSettingsFile();
    if (file.existsAsFile())
    {
        auto json = juce::JSON::parse(file);
        if (json.isObject() && json.hasProperty("aiProvider"))
        {
            auto p = json["aiProvider"].toString().toLowerCase();
            if (p == "openai") return AIProvider::OpenAI;
            if (p == "claude") return AIProvider::Claude;
            if (p == "deepseek") return AIProvider::DeepSeek;
            return AIProvider::Gemini;
        }
    }
    return AIProvider::Gemini;
}

void AIService::setProvider(AIProvider provider)
{
    auto file = getSettingsFile();
    juce::var json;
    if (file.existsAsFile())
        json = juce::JSON::parse(file);
    if (!json.isObject())
        json = juce::var(new juce::DynamicObject());

    if (auto* obj = json.getDynamicObject())
    {
        obj->setProperty("aiProvider", getProviderKeyName(provider));
    }
    file.replaceWithText(juce::JSON::toString(json));
}

juce::String AIService::getProviderName(AIProvider provider)
{
    switch (provider)
    {
        case AIProvider::Gemini:   return "Google Gemini (Flash)";
        case AIProvider::OpenAI:   return "OpenAI (ChatGPT)";
        case AIProvider::Claude:   return "Anthropic Claude";
        case AIProvider::DeepSeek: return "DeepSeek";
    }
    return "Google Gemini";
}

juce::String AIService::getProviderKeyName(AIProvider provider)
{
    switch (provider)
    {
        case AIProvider::Gemini:   return "gemini";
        case AIProvider::OpenAI:   return "openai";
        case AIProvider::Claude:   return "claude";
        case AIProvider::DeepSeek: return "deepseek";
    }
    return "gemini";
}

juce::String AIService::getApiKey(AIProvider provider)
{
    auto file = getSettingsFile();
    if (file.existsAsFile())
    {
        auto json = juce::JSON::parse(file);
        if (json.isObject())
        {
            juce::Identifier propId("aiApiKey_" + getProviderKeyName(provider));
            if (json.hasProperty(propId))
            {
                return json[propId].toString().trim();
            }
            // Fallback to generic aiApiKey if it's the current provider
            if (json.hasProperty("aiApiKey") && getProvider() == provider)
            {
                return json["aiApiKey"].toString().trim();
            }
        }
    }
    return {};
}

void AIService::setApiKey(AIProvider provider, const juce::String& key)
{
    auto file = getSettingsFile();
    juce::var json;
    if (file.existsAsFile())
        json = juce::JSON::parse(file);
    if (!json.isObject())
        json = juce::var(new juce::DynamicObject());

    if (auto* obj = json.getDynamicObject())
    {
        juce::Identifier propId("aiApiKey_" + getProviderKeyName(provider));
        if (key.trim().isNotEmpty())
            obj->setProperty(propId, key.trim());
        else
            obj->removeProperty(propId);
    }
    file.replaceWithText(juce::JSON::toString(json));
}

bool AIService::isEnabled()
{
    auto file = getSettingsFile();
    if (file.existsAsFile())
    {
        auto json = juce::JSON::parse(file);
        if (json.isObject() && json.hasProperty("aiEnabled"))
        {
            return (bool)json["aiEnabled"];
        }
    }
    // Default to enabled if an API key is already configured
    return isConfigured();
}

void AIService::setEnabled(bool enabled)
{
    auto file = getSettingsFile();
    juce::var json;
    if (file.existsAsFile())
        json = juce::JSON::parse(file);
    if (!json.isObject())
        json = juce::var(new juce::DynamicObject());

    if (auto* obj = json.getDynamicObject())
    {
        obj->setProperty("aiEnabled", enabled);
    }
    file.replaceWithText(juce::JSON::toString(json));
}

bool AIService::isConfigured()
{
    return getApiKey(getProvider()).isNotEmpty();
}

juce::String AIService::buildPrompt(const CurrentTrackContext& context)
{
    juce::String prompt;
    prompt << "Actúa como un DJ y productor musical experto en mezclas armónicas para festivales y clubes.\n";
    prompt << "La pista actualmente en reproducción es:\n";
    prompt << "- Título: \"" << (context.title.isNotEmpty() ? context.title : "Pista de prueba") << "\"\n";
    if (context.artist.isNotEmpty())
        prompt << "- Artista: \"" << context.artist << "\"\n";
    if (context.bpm > 0.0)
        prompt << "- BPM Detectado: " << juce::String(context.bpm, 1) << "\n";
    if (context.key.isNotEmpty() && context.key != "--")
        prompt << "- Tonalidad Armónica (Camelot): " << context.key << "\n";
    prompt << "- Fuente: " << (context.isVideo ? "Video de YouTube" : "Audio Local") << "\n\n";

    prompt << "Recomienda exactamente 5 canciones del mismo género musical o estilos compatibles (por ejemplo Tech House, Melodic Techno, Reggaeton, Afrobeats, etc.), "
           << "con un tempo armónicamente mezclable (+/- 4 BPM) y energía fluida para una transición perfecta de DJ.\n";
    prompt << "Responde ÚNICAMENTE un objeto JSON válido con la clave 'recommendations', siguiendo exactamente esta estructura:\n"
           << "{\n"
           << "  \"recommendations\": [\n"
           << "    {\n"
           << "      \"title\": \"Nombre de la canción\",\n"
           << "      \"artist\": \"Nombre del artista\",\n"
           << "      \"bpm\": 126,\n"
           << "      \"key\": \"8A\",\n"
           << "      \"genre\": \"Tech House\",\n"
           << "      \"reason\": \"Mismo groove de percusión y tono armónico perfecto\"\n"
           << "    }\n"
           << "  ]\n"
           << "}\n";

    return prompt;
}

std::vector<AIRecommendation> AIService::parseResponse(AIProvider provider, const juce::String& rawJson, juce::String& outError)
{
    juce::ignoreUnused(provider);
    std::vector<AIRecommendation> list;

    auto parsed = juce::JSON::parse(rawJson);
    if (!parsed.isObject())
    {
        outError = "Respuesta no válida del servidor de IA.";
        return list;
    }

    juce::String textContent;

    // Check Gemini format: candidates[0].content.parts[0].text
    if (parsed.hasProperty("candidates"))
    {
        auto candidates = parsed["candidates"];
        if (candidates.isArray() && candidates.size() > 0)
        {
            auto first = candidates[0];
            if (first.hasProperty("content") && first["content"].hasProperty("parts"))
            {
                auto parts = first["content"]["parts"];
                if (parts.isArray() && parts.size() > 0)
                {
                    textContent = parts[0]["text"].toString();
                }
            }
        }
    }
    // Check OpenAI / DeepSeek format: choices[0].message.content
    else if (parsed.hasProperty("choices"))
    {
        auto choices = parsed["choices"];
        if (choices.isArray() && choices.size() > 0)
        {
            auto first = choices[0];
            if (first.hasProperty("message") && first["message"].hasProperty("content"))
            {
                textContent = first["message"]["content"].toString();
            }
        }
    }
    // Check Claude format: content[0].text
    else if (parsed.hasProperty("content"))
    {
        auto content = parsed["content"];
        if (content.isArray() && content.size() > 0)
        {
            textContent = content[0]["text"].toString();
        }
    }
    // Direct recommendations key
    else if (parsed.hasProperty("recommendations"))
    {
        textContent = rawJson;
    }

    if (textContent.isEmpty())
    {
        if (parsed.hasProperty("error"))
        {
            outError = parsed["error"]["message"].toString();
        }
        else
        {
            outError = "No se pudo interpretar el contenido generado por la IA.";
        }
        return list;
    }

    // Strip markdown code fences if present (e.g. ```json ... ```)
    juce::String cleanText = textContent.trim();
    if (cleanText.startsWith("```json"))
    {
        cleanText = cleanText.substring(7);
        if (cleanText.endsWith("```"))
            cleanText = cleanText.dropLastCharacters(3).trim();
    }
    else if (cleanText.startsWith("```"))
    {
        cleanText = cleanText.substring(3);
        if (cleanText.endsWith("```"))
            cleanText = cleanText.dropLastCharacters(3).trim();
    }
    cleanText = cleanText.trim();

    auto recsJson = juce::JSON::parse(cleanText);
    if (!recsJson.isObject() || !recsJson.hasProperty("recommendations"))
    {
        outError = "Formato JSON de recomendaciones no reconocido.";
        return list;
    }

    auto items = recsJson["recommendations"];
    if (!items.isArray())
    {
        outError = "La lista de recomendaciones está vacía.";
        return list;
    }

    for (int i = 0; i < items.size(); ++i)
    {
        auto it = items[i];
        if (!it.isObject()) continue;

        AIRecommendation rec;
        rec.title  = it["title"].toString();
        rec.artist = it["artist"].toString();
        rec.bpm    = (double)it["bpm"];
        rec.key    = it["key"].toString();
        rec.genre  = it["genre"].toString();
        rec.reason = it["reason"].toString();

        if (rec.title.isNotEmpty())
            list.push_back(rec);
    }

    return list;
}

void AIService::requestRecommendationsAsync(
    const CurrentTrackContext& context,
    std::function<void(const std::vector<AIRecommendation>&, const juce::String&)> onComplete)
{
    auto provider = getProvider();
    auto apiKey = getApiKey(provider);

    if (apiKey.isEmpty())
    {
        juce::MessageManager::callAsync([onComplete]() {
            onComplete({}, "Clave API no configurada para el proveedor seleccionado.");
        });
        return;
    }

    std::thread([context, provider, apiKey, onComplete]() {
        juce::String promptText = buildPrompt(context);
        juce::String endpoint;
        juce::String postData;
        juce::String authHeader;

        juce::var payload(new juce::DynamicObject());

        if (provider == AIProvider::Gemini)
        {
            endpoint = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + apiKey;

            // Gemini payload
            juce::var partsArray;
            juce::var partObj(new juce::DynamicObject());
            partObj.getDynamicObject()->setProperty("text", promptText);
            partsArray.append(partObj);

            juce::var contentObj(new juce::DynamicObject());
            contentObj.getDynamicObject()->setProperty("parts", partsArray);
            juce::var contentsArray;
            contentsArray.append(contentObj);

            payload.getDynamicObject()->setProperty("contents", contentsArray);

            juce::var genConfig(new juce::DynamicObject());
            genConfig.getDynamicObject()->setProperty("responseMimeType", "application/json");
            payload.getDynamicObject()->setProperty("generationConfig", genConfig);
        }
        else if (provider == AIProvider::OpenAI)
        {
            endpoint = "https://api.openai.com/v1/chat/completions";
            authHeader = "-H \"Authorization: Bearer " + apiKey + "\"";

            payload.getDynamicObject()->setProperty("model", "gpt-4o-mini");

            juce::var messagesArray;
            juce::var msgObj(new juce::DynamicObject());
            msgObj.getDynamicObject()->setProperty("role", "user");
            msgObj.getDynamicObject()->setProperty("content", promptText);
            messagesArray.append(msgObj);

            payload.getDynamicObject()->setProperty("messages", messagesArray);

            juce::var respFmt(new juce::DynamicObject());
            respFmt.getDynamicObject()->setProperty("type", "json_object");
            payload.getDynamicObject()->setProperty("response_format", respFmt);
        }
        else if (provider == AIProvider::DeepSeek)
        {
            endpoint = "https://api.deepseek.com/chat/completions";
            authHeader = "-H \"Authorization: Bearer " + apiKey + "\"";

            payload.getDynamicObject()->setProperty("model", "deepseek-chat");

            juce::var messagesArray;
            juce::var msgObj(new juce::DynamicObject());
            msgObj.getDynamicObject()->setProperty("role", "user");
            msgObj.getDynamicObject()->setProperty("content", promptText);
            messagesArray.append(msgObj);

            payload.getDynamicObject()->setProperty("messages", messagesArray);

            juce::var respFmt(new juce::DynamicObject());
            respFmt.getDynamicObject()->setProperty("type", "json_object");
            payload.getDynamicObject()->setProperty("response_format", respFmt);
        }
        else if (provider == AIProvider::Claude)
        {
            endpoint = "https://api.anthropic.com/v1/messages";
            authHeader = "-H \"x-api-key: " + apiKey + "\" -H \"anthropic-version: 2023-06-01\"";

            payload.getDynamicObject()->setProperty("model", "claude-3-5-haiku-20241022");
            payload.getDynamicObject()->setProperty("max_tokens", 1024);

            juce::var messagesArray;
            juce::var msgObj(new juce::DynamicObject());
            msgObj.getDynamicObject()->setProperty("role", "user");
            msgObj.getDynamicObject()->setProperty("content", promptText);
            messagesArray.append(msgObj);

            payload.getDynamicObject()->setProperty("messages", messagesArray);
        }

        postData = juce::JSON::toString(payload);

        // Write payload to temp file for curl execution
        auto tempFile = juce::File::createTempFile("ai_req_");
        tempFile.replaceWithText(postData);

        auto respFile = juce::File::createTempFile("ai_resp_");

        juce::String curlCmd = "curl -s -L --max-time 15 -X POST \"" + endpoint + "\" "
                             + "-H \"Content-Type: application/json\" "
                             + authHeader + " "
                             + "--data-binary \"@" + tempFile.getFullPathName() + "\" "
                             + "-o \"" + respFile.getFullPathName() + "\"";

        int res = system(curlCmd.toRawUTF8());
        tempFile.deleteFile();

        juce::String rawResponse;
        if (res == 0 && respFile.existsAsFile())
        {
            rawResponse = respFile.loadFileAsString();
            respFile.deleteFile();
        }

        juce::String parseError;
        std::vector<AIRecommendation> recs;
        if (rawResponse.isNotEmpty())
        {
            recs = parseResponse(provider, rawResponse, parseError);
        }
        else
        {
            parseError = "No hubo respuesta del servidor o tiempo de espera agotado.";
        }

        juce::MessageManager::callAsync([onComplete, recs, parseError]() {
            onComplete(recs, parseError);
        });
    }).detach();
}

void AIService::resolveLocalMatches(std::vector<AIRecommendation>& recs,
                                    const std::vector<juce::File>& localLibraryFiles)
{
    for (auto& r : recs)
    {
        r.isLocal = false;
        r.localFile = juce::File();

        auto cleanTitle = r.title.toLowerCase().trim();
        auto cleanArtist = r.artist.toLowerCase().trim();

        for (const auto& f : localLibraryFiles)
        {
            auto fname = f.getFileNameWithoutExtension().toLowerCase();
            // Check if both artist and title are inside the filename
            if (fname.contains(cleanTitle) || 
               (cleanArtist.isNotEmpty() && fname.contains(cleanArtist) && fname.contains(cleanTitle.substring(0, 5))))
            {
                r.isLocal = true;
                r.localFile = f;
                break;
            }
        }
    }
}
