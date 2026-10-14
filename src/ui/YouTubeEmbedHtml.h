#pragma once

#include <juce_core/juce_core.h>

// Wrapper HTML único para el reproductor YouTube en las 3 plataformas.
// - Controles propios de YouTube bloqueados: controls=0 + div transparente
//   sobre el iframe (el deck es la única autoridad: PLAY/CUE/jog).
// - Control desde C++ vía postMessage (play/pause/setVolume/seekTo).
// - Cabeceras estrictas de origen y referer para evitar Error 153 en WebViews.
juce::String buildYouTubeEmbedHtml(const juce::String& videoId, bool autoplay, const juce::String& originHost = {});
