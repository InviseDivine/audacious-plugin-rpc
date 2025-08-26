#include <iostream>
#include <string.h>
#include <format>

#include <libaudcore/drct.h>
#include <libaudcore/i18n.h>
#include <libaudcore/plugin.h>
#include <libaudcore/hook.h>
#include <libaudcore/audstrings.h>
#include <libaudcore/tuple.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include <discord_rpc.h>
#include <cpr/cpr.h>

#define EXPORT __attribute__((visibility("default")))
#define APPLICATION_ID "1408523003532279828"

class RPCPlugin : public GeneralPlugin {
public:
    static const char about[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;
    
    // Info about plugin
    static constexpr PluginInfo info = {
        N_("Discord RPC Plugin"),
        "audacious-plugin-rpc",
        about,
        &prefs
    };

    constexpr RPCPlugin() : GeneralPlugin (info, false) {}

    bool init();
    void cleanup();
};

EXPORT RPCPlugin aud_plugin_instance;

DiscordEventHandlers handlers;

std::string tempAlbum = "";
std::string albumCover = "";

std::string replaceSpaces(const std::string& originalString, const std::string& replacement) {
    std::string newString = ""; // Initialize an empty string to store the result
    for (char c : originalString) { // Iterate through each character of the original string
        if (c == ' ') { // If the current character is a space
            newString += replacement; // Append the replacement string
        } else {
            newString += c; // Append the character as is
        }
    }
    return newString; // Return the modified string
}

void set_album_cover(std::string artist, std::string album) {
    // Formatting strings to get working url
    std::string formattedArtist = replaceSpaces(artist, "%20");
    std::string formattedAlbum = replaceSpaces(album, "%20");

    // Creating url
    std::string url = "http://ws.audioscrobbler.com/2.0/?method=album.getinfo";
    url.append("&api_key=5a499a2308896d61c7a8977e9bd3936f");
    url.append(std::format("&artist={}&album={}", formattedArtist, formattedAlbum));

    // Sending GET request
    cpr::Response r = cpr::Get(cpr::Url{url});
    
    // Getting url to mega album cover
    std::string megastr = "<image size=\"mega\">";
    std::string megaendstr = "</image>";

    auto megapos = r.text.find(megastr);
    auto megaend = r.text.find(megaendstr, megapos);
    
    // Our album cover string
    std::string albumCvr = r.text.substr(megapos + megastr.size(), megaend - megapos - megastr.size());

    // Set global url to new url
    albumCover = albumCvr;
}
void init_discord() {
    // Initializating Discord RPC
    memset(&handlers, 0, sizeof(handlers));
    Discord_Initialize(APPLICATION_ID, &handlers, 1, NULL);
}

void cleanup_discord() {
    // Clearing and shutting down Discord RPC
    Discord_ClearPresence();
    Discord_Shutdown();
}

void title_changed() {
    // Do nothing when audacious doesn't ready
    if (!aud_drct_get_ready()) {
        return;
    }

    // Creating presence
    DiscordRichPresence presence;
    memset(&presence, 0, sizeof(presence));
    
    // Some vars
    bool isHidePresence = aud_get_bool(nullptr, "isHidePresence");

    // Current song data
    Tuple tuple = aud_drct_get_tuple();

    String title = tuple.get_str(Tuple::Title);
    String artist = tuple.get_str(Tuple::Artist);
    String album = tuple.get_str(Tuple::Album);

    // Setting information about artist and album
    presence.state = artist;
    presence.largeImageText = album;

    // Setting start timestamp
    presence.startTimestamp = time(0);

    // Setting activity to listening
    presence.activityType = DISCORD_ACTIVITY_LISTENING;
    
    // Setting album cover
    if (std::string(album) != tempAlbum) {
        set_album_cover(std::string(artist), std::string(album));
        tempAlbum = std::string(album);
    }
    presence.largeImageKey = albumCover.c_str();
    presence.largeImageText = album;

    if (!aud_drct_get_paused()) {
        // Setting information about song title and song length
        presence.details = title;

        // Audacious provides song length in milliseconds, so we divide by 1000
        presence.endTimestamp = time(0) + (aud_drct_get_length() / 1000);

        // Updating activity status
        Discord_UpdatePresence(&presence);
    } else {
        // If isHidePresence true, just clearing Discord RPC
        if (isHidePresence) {
            Discord_ClearPresence();
        } else {
            // Clearing song length
            presence.endTimestamp = 0;
            
            // Creating std::string to append "paused" prefix
            std::string buf(title);
            presence.details = buf.append(" (paused)").c_str();

            // Updating activity status
            Discord_UpdatePresence(&presence);
        }
    }
}

void update_title_presence(void*, void*) {
    title_changed();
}

void open_github() {
    // Linux rofls
    system("xdg-open https://github.com/InviseDivine/audacious-plugin-rpc");
}

bool RPCPlugin::init() {
    // Associating this events (hooks) with our function and initing Discord RPC
    init_discord();

    hook_associate("playback ready", update_title_presence, nullptr);
    hook_associate("playback end", update_title_presence, nullptr);
    hook_associate("playback stop", update_title_presence, nullptr);
    hook_associate("playback pause", update_title_presence, nullptr);
    hook_associate("playback unpause", update_title_presence, nullptr);
    hook_associate("title change", update_title_presence, nullptr);
    hook_associate("tuple change", update_title_presence, nullptr);

    return true;
}

void RPCPlugin::cleanup() {
    // Disassociating this events (hooks) with our function and cleaning discord
    cleanup_discord();

    hook_dissociate("playback ready", update_title_presence);
    hook_dissociate("playback end", update_title_presence);
    hook_dissociate("playback stop", update_title_presence);
    hook_dissociate("playback pause", update_title_presence);
    hook_dissociate("playback unpause", update_title_presence);
    hook_dissociate("title change", update_title_presence);
    hook_dissociate("tuple change", update_title_presence);
}

// About developers
const char RPCPlugin::about[] = N_(\
"Discord RPC music status plugin\n\n\
Written by: Derzsi Daniel <daniel@tohka.us>\n\n\
Fork by: InviseDivine <invisedivine@sffempire.ru>");

// Settings
const PreferencesWidget RPCPlugin::widgets[] =
{
    WidgetCheck (N_("Hide presence when pause"),
      WidgetBool(0, "isHidePresence", title_changed)),

    WidgetButton(N_("Fork on GitHub"),
        {open_github})
};

const PluginPreferences RPCPlugin::prefs = {{ widgets }};