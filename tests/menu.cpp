#define main splitemuEntry
#include "../src/main.cpp"
#undef main
#undef NDEBUG
#include <cassert>
#include <iostream>

static int button(Uint8 code, SDL_JoystickID id) {
    SDL_Event ev{}; ev.type=SDL_CONTROLLERBUTTONDOWN;
    ev.cbutton.button=code; ev.cbutton.which=id;
    return launcherHandleEvent(ev);
}
static int key(SDL_Keycode code) {
    SDL_Event ev{}; ev.type=SDL_KEYDOWN; ev.key.keysym.sym=code;
    return launcherHandleEvent(ev);
}
int main() {
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER)==0);
    std::array<SDL_JoystickID,4> ids;
    for(int i=0;i<4;++i) {
        int device=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER,
                                            SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        assert(device>=0);
        assignController(device);
        assert(g_players[i].pad);
        ids[i]=g_players[i].padId;
        g_app.profileName[i]="PROFILE " + std::to_string(i+1);
    }
    syncProfileNames();
    g_app.romList={"test.gba", "translated.gba"};
    g_app.launcherPlayers=4;
    assert(launcherRows().front() == LR_ROM_ALL);
    button(SDL_CONTROLLER_BUTTON_DPAD_RIGHT, ids[0]);
    assert(std::all_of(g_app.launcherRom, g_app.launcherRom + MAX_PLAYERS,
                       [](int rom) { return rom == 1; }));
    launcherAdjust(LR_P1, -1);
    assert(!launcherUsesOneRom());
    key(SDLK_RETURN);
    assert(launcherUsesOneRom() && g_app.launcherRom[0] == 0);
    // Enter opens a picker; navigation/cancellation cannot change assignments.
    auto rows=launcherRows();
    g_app.launcherSel=int(std::find(rows.begin(),rows.end(),LR_PROFILE0)-rows.begin());
    button(SDL_CONTROLLER_BUTTON_A,ids[0]);
    assert(g_app.profilePickerScreen==0 && !g_app.editingName);
    button(SDL_CONTROLLER_BUTTON_DPAD_DOWN,ids[0]);
    assert(g_app.profilePickerSel==1 && g_app.screenProfiles[0]==0);
    button(SDL_CONTROLLER_BUTTON_B,ids[0]);
    assert(g_app.profilePickerScreen==-1 && g_app.screenProfiles[0]==0);
    launcherActivate(LR_PROFILE0);
    button(SDL_CONTROLLER_BUTTON_DPAD_DOWN,ids[0]);
    button(SDL_CONTROLLER_BUTTON_A,ids[0]);
    assert(g_app.profilePickerScreen==-1);
    assert(g_app.screenProfiles[0]==1 && g_app.screenProfiles[1]==0);
    assert(g_players[0].padId==ids[1] && g_players[1].padId==ids[0]);
    // Inactive profiles can be moved to an active screen without duplication.
    g_app.launcherPlayers=2;
    launcherActivate(LR_PROFILE0);
    g_app.profilePickerSel=3;
    key(SDLK_RETURN);
    assert(g_app.screenProfiles[0]==3 && g_app.screenProfiles[3]==1);
    assert(g_players[0].padId==ids[3]);
    assert(session::isPermutation(g_app.screenProfiles));
    // Renaming remains available, with B cancelling only the edit, then picker.
    launcherActivate(LR_PROFILE0);
    g_app.profilePickerSel=4;
    key(SDLK_RETURN);
    assert(g_app.editingName);
    g_app.editBuffer="UNSAVED";
    button(SDL_CONTROLLER_BUTTON_B,ids[3]);
    assert(!g_app.editingName && g_app.profilePickerScreen==0);
    assert(g_app.profileName[3]=="PROFILE 4");
    button(SDL_CONTROLLER_BUTTON_B,ids[3]);
    assert(g_app.profilePickerScreen==-1);
    // D-pad navigates controller list, never assigns a controller.
    openPadAssign();
    button(SDL_CONTROLLER_BUTTON_DPAD_DOWN,ids[3]);
    assert(g_app.padAssignSel==1 && !g_app.padAssignWaiting);
    assert(g_players[1].padId==ids[0]);
    // First A opens prompt without moving the controller that opened it.
    button(SDL_CONTROLLER_BUTTON_A,ids[3]);
    assert(g_app.padAssignWaiting && g_players[1].padId==ids[0]);
    button(SDL_CONTROLLER_BUTTON_X,ids[2]);
    key(SDLK_RETURN);
    assert(g_app.padAssignWaiting && g_players[1].padId==ids[0]);
    // Next A chooses a different controller and swaps previous owner.
    g_players[1].padKeys=7; g_players[2].padKeys=9;
    button(SDL_CONTROLLER_BUTTON_A,ids[2]);
    assert(!g_app.padAssignWaiting && g_app.padAssignSel==1);
    assert(g_players[1].padId==ids[2] && g_players[2].padId==ids[0]);
    assert(g_players[1].padKeys==0 && g_players[2].padKeys==0);
    // Already-assigned controller is an idempotent selection.
    key(SDLK_RETURN);
    button(SDL_CONTROLLER_BUTTON_A,ids[2]);
    assert(!g_app.padAssignWaiting && g_players[1].padId==ids[2]);
    // B cancels capture, then closes list; it cannot leak into root Quit.
    key(SDLK_RETURN);
    button(SDL_CONTROLLER_BUTTON_B,ids[3]);
    assert(!g_app.padAssignWaiting && g_app.padAssignOpen);
    button(SDL_CONTROLLER_BUTTON_B,ids[3]);
    assert(!g_app.padAssignOpen);
    assert(button(SDL_CONTROLLER_BUTTON_B,ids[3])==key(SDLK_ESCAPE));
    assert(key(SDLK_ESCAPE)==2);
    // Missing/disconnected device cannot claim a profile.
    openPadAssign(); key(SDLK_RETURN);
    button(SDL_CONTROLLER_BUTTON_A,-123);
    assert(g_app.padAssignWaiting);
    button(SDL_CONTROLLER_BUTTON_B,ids[3]);
    button(SDL_CONTROLLER_BUTTON_B,ids[3]);
    g_app.editingName=true; g_app.editingIndex=0; g_app.editBuffer="CANCEL ME";
    SDL_ControllerButtonEvent back{}; back.button=SDL_CONTROLLER_BUTTON_B;
    bool quit=false; menuHandlePad(back,&quit);
    assert(!g_app.editingName && !quit && g_app.profileName[3]=="PROFILE 4");
    for(auto& p:g_players) { SDL_GameControllerClose(p.pad); p.pad=nullptr; p.padId=-1; }
    SDL_Quit();
    std::cout << "Controller navigation, profile popup/swaps and A-only capture passed\n";
}
