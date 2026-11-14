#define main splitemuEntry
#include "../src/main.cpp"
#undef main
#undef NDEBUG
#include <cassert>
#include <iostream>

static std::string read(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
int main() {
    auto dir = fs::temp_directory_path() / ("splitemu-profiles-" + std::to_string(SDL_GetPerformanceCounter()));
    fs::create_directories(dir);
#ifdef _WIN32
    _putenv_s("APPDATA", dir.string().c_str());
#else
    setenv("HOME",dir.string().c_str(),1);
#endif
    auto rom = (dir / "Emerald.gba").string();
    std::ofstream(rom) << "ROM";
    for(int i=0;i<4;++i)
        std::ofstream(stripExtension(rom)+".p"+std::to_string(i+1)+".sav") << "SAVE-" << i;
    g_app.playerName[1] = "HANNA";
    loadProfiles();
    registerProfileSaves(rom);
    Player player; player.romPath=rom;
    std::string hanna = savePathFor(player,1,true);
    assert(read(hanna)=="SAVE-1");
    session::cycleAssignment(g_app.screenProfiles,0,1);
    syncProfileNames();
    assert(g_app.playerName[0]=="HANNA");
    assert(savePathFor(player,0,false)==hanna);
    assert(statePathFor(player,0)==stripExtension(rom)+".p2.ss1");
    g_app.editingIndex=0; g_app.editBuffer="HANNA NEW";
    commitProfileName();
    assert(savePathFor(player,0,true)==hanna && read(hanna)=="SAVE-1");
    g_app.linkMode=3; g_app.cableProfiles={0,2,1,3};
    assert(saveProfiles());
    g_app.profileSaves.clear(); g_app.profileName[1]=""; g_app.screenProfiles={0,1,2,3};
    loadProfiles(); syncProfileNames();
    assert(g_app.playerName[0]=="HANNA NEW");
    assert((g_app.linkMode==3 && g_app.cableProfiles==std::array<int,4>({0,2,1,3})));
    assert(savePathFor(player,0,true)==hanna);
    // Legacy single-player save belongs to original profile 3, not current screen.
    auto single=(dir / "Ruby.gba").string();
    std::ofstream(stripExtension(single)+".sav") << "SINGLE";
    g_app.savedRomPath[2]=single;
    registerProfileSaves(single);
    assert(g_app.profileSaves.at(romKey(single))[2]==stripExtension(single)+".sav");
    assert(g_app.profileSaves.at(romKey(single))[0]!=stripExtension(single)+".sav");
    assert(read(stripExtension(single)+".sav")=="SINGLE");
    // Launcher validation blocks inactive cable members and controller Start uses it too.
    g_app.launcherPlayers=2; g_app.screenProfiles={0,1,2,3};
    g_app.linkMode=1; g_app.cableProfiles={0,2,1,3};
    assert(launcherActivate(LR_START)==0);
    launcherAdjust(LR_CABLE1,-1);
    assert(launcherActivate(LR_START)==1);
    // Corrupt/empty metadata must never silently rebind existing saves.
    std::ofstream(profilesPath(),std::ios::trunc) << "screens 0 0 2 3\n";
    assert(!loadProfiles());
    std::ofstream(profilesPath(),std::ios::trunc).close();
    assert(!loadProfiles());
    fs::remove_all(dir);
    std::cout << "Legacy saves, reassignment, rename, savestates and persistence passed\n";
}
