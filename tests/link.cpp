// Exercise the actual mGBA serial drivers with four distinct homebrew senders.
#define main splitemuEntry
#include "../src/main.cpp"
#undef main
#undef NDEBUG
#include <cassert>
#include <iostream>
int main(int argc, char** argv) {
    if(argc != 7) return 2; // mode, cable order, four tagged ROMs
    auto dir = fs::temp_directory_path() / ("splitemu-link-" + std::to_string(SDL_GetPerformanceCounter()));
    fs::create_directories(dir);
#ifdef _WIN32
    _putenv_s("APPDATA",dir.string().c_str());
#else
    setenv("HOME",dir.string().c_str(),1);
#endif
    s_logger.log=quietLog; mLogSetDefaultLogger(&s_logger);
    assert(SDL_Init(SDL_INIT_TIMER)==0);
    g_app.linkMode=atoi(argv[1]);
    for(int i=0;i<4;++i) g_app.cableProfiles[i]=argv[2][i]-'0';
    g_app.freeRun=true; // tests use no wall-clock/audio pacing
    g_app.numPlayers=4;
    for(int i=0;i<4;++i) {
        auto path=dir / ("sender"+std::to_string(i)+".gba");
        fs::copy_file(argv[i+3],path);
        assert(bootPlayer(g_players[i],i,path.string(),false));
    }
    attachLink();
    assert(g_app.cableCount==(g_app.linkMode==3?2:1));
    std::array<bool,4> received{};
    bool complete=false;
    for(int attempt=0;attempt<600 && !complete;++attempt) {
        SDL_Delay(5);
        interruptAll();
        complete=true;
        for(int c=0;c<g_app.cableCount;++c) {
            const auto& screens=g_app.cables[c].screens;
            for(int screen:screens) {
                auto* core=g_players[screen].core;
                bool matches=true;
                // Registers may be 0xFFFF while the next transfer is in flight.
                // Require a completed packet from every screen, not one timed snapshot.
                for(int peer=0;peer<4;++peer) {
                    uint16_t expected=peer<int(screens.size()) ? ((0xA0+screens[peer])<<8)|peer : 0xFFFF;
                    uint16_t value=core->busRead16(core,0x04000120+peer*2);
                    // At boot registers contain zero, and the first queued send may
                    // predate cable-ID assignment. Any identifiable sender must
                    // still belong to this cable and this peer position.
                    if ((value >> 8) >= 0xA0 && (value >> 8) <= 0xA3) {
                        if (peer >= int(screens.size()) || (value >> 8) != (expected >> 8)) {
                            std::cerr << "Foreign sender on cable " << c << " screen " << screen
                                      << " peer " << peer << " value " << std::hex << value << "\n";
                            std::abort();
                        }
                    }
                    matches &= value==expected;
                }
                received[screen]=received[screen] || matches;
                complete &= received[screen];
                assert(g_players[screen].node->id==int(std::find(screens.begin(),screens.end(),screen)-screens.begin()));
            }
        }
        continueAll();
    }
    if(!complete) {
        std::cerr << "Timed out waiting for complete serial packets\n";
        std::abort();
    }
    if(g_app.linkMode==1) {
        for(int i=0;i<4;++i) if(!g_players[i].node)
            assert(g_players[i].frameCount.load()>0); // unlinked screens keep running
    }
    shutdownPlayers(); // regression: all nodes outlive all driver unload callbacks
    SDL_Quit();
    fs::remove_all(dir);
    std::cout << "Serial transfer and clean shutdown passed for mode " << argv[1]
              << " cable order " << argv[2] << "\n";
}
