#include "YouTubeEmbedHtml.h"

juce::String buildYouTubeEmbedHtml(const juce::String& videoId, bool autoplay)
{
    juce::String html;
    html << "<!DOCTYPE html><html><head>"
         << "<meta name='viewport' content='width=device-width, initial-scale=1'>"
         << "<style>"
         << "*{margin:0;padding:0;box-sizing:border-box}"
         << "html,body{width:100%;height:100%;overflow:hidden;background:#000}"
         << "#player{width:100%;height:100%;position:absolute;top:0;left:0;border:none}"
         // ponytail: blocker siempre activo; sin él, el clic sobre el video
         // alterna play/pause y el botón del deck nace desincronizado
         << "#blocker{position:absolute;top:0;left:0;width:100%;height:100%;z-index:10;background:transparent}"
         << "</style></head><body>"
         << "<iframe id='player' src='https://www.youtube-nocookie.com/embed/" << videoId
         << "?autoplay=" << (autoplay ? "1" : "0")
         << "&controls=0&disablekb=1&fs=0&rel=0&enablejsapi=1&origin=http://localhost:3000' "
         << "allow='autoplay; encrypted-media; picture-in-picture'></iframe>"
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
