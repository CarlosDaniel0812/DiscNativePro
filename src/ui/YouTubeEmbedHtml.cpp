#include "YouTubeEmbedHtml.h"

juce::String buildYouTubeEmbedHtml(const juce::String& videoId, bool autoplay, const juce::String& originHost)
{
    juce::String html;
    html << "<!DOCTYPE html><html><head>"
         << "<meta charset='utf-8'>"
         << "<meta name='viewport' content='width=device-width, initial-scale=1'>"
         << "<meta name='referrer' content='strict-origin-when-cross-origin'>"
         << "<style>"
         << "*{margin:0;padding:0;box-sizing:border-box}"
         << "html,body{width:100%;height:100%;overflow:hidden;background:#000}"
         << "#player{width:100%;height:100%;position:absolute;top:0;left:0;border:none}"
         << "#blocker{position:absolute;top:0;left:0;width:100%;height:100%;z-index:10;background:transparent}"
         << "</style></head><body>";

    juce::String iframeSrc = "https://www.youtube.com/embed/" + videoId
                           + "?autoplay=" + (autoplay ? "1" : "0")
                           + "&controls=0&disablekb=1&fs=0&rel=0&enablejsapi=1";

    if (originHost.isNotEmpty())
        iframeSrc += "&origin=" + originHost;

    html << "<iframe id='player' src='" << iframeSrc << "' "
         << "referrerpolicy='strict-origin-when-cross-origin' "
         << "allow='accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture' "
         << "allowfullscreen></iframe>"
         << "<div id='blocker'></div>"
         << "<script>"
         << "function ytmsg(o){try{var f=document.getElementById('player');"
         << "if(f&&f.contentWindow)f.contentWindow.postMessage(JSON.stringify(o),'*');}catch(e){}}"
         << "function play(){ytmsg({event:'command',func:'unMute',args:''});"
         << "ytmsg({event:'command',func:'playVideo',args:''});}"
         << "function pause(){ytmsg({event:'command',func:'pauseVideo',args:''});}"
         << "function setVolume(v){if(v<=0){ytmsg({event:'command',func:'mute',args:''});}"
         << "else{ytmsg({event:'command',func:'unMute',args:''});"
         << "ytmsg({event:'command',func:'setVolume',args:[v]});}}"
         << "function seek(s){ytmsg({event:'command',func:'seekTo',args:[s,true]});}"
         << "</script></body></html>";

    return html;
}
