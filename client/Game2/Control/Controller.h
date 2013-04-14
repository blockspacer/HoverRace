
// Controller.h
//
// Copyright (c) 2010 Ryan Curtin.
// Copyright (c) 2013 Michael Imamura.
//
// Licensed under GrokkSoft HoverRace SourceCode License v1.0(the "License");
// you may not use this file except in compliance with the License.
//
// A copy of the license should have been attached to the package from which
// you have taken this file. If you can not find the license you can not use
// this file.
//
//
// The author makes no representations about the suitability of
// this software for any purpose.  It is provided "as is" "AS IS",
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied.
//
// See the License for the specific language governing permissions
// and limitations under the License.

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <vector>
#include <string>

#include <SDL2/SDL.h>

#include "../../../engine/Util/Config.h"
#include "../../../engine/Util/OS.h"
#include "../../../engine/MainCharacter/MainCharacter.h"
#include "../HoverScript/HighConsole.h"
#include "../Observer.h"

#include "Action.h"
#include "ControlAction.h"

#define	CTL_MOTOR_ON	1
#define CTL_LEFT		2
#define CTL_RIGHT		3
#define CTL_JUMP		4
#define CTL_BRAKE		5
#define CTL_FIRE		6
#define CTL_WEAPON		7
#define CTL_LOOKBACK	8

#define AXIS_X			1
#define AXIS_Y			2
#define AXIS_Z			3

#define UNASSIGNED		-1

#define SET_CONTROL		(WM_USER + 1)

namespace HoverRace {
namespace Client {
namespace Control {

class InputHandler;
typedef std::shared_ptr<InputHandler> InputHandlerPtr;
class UiHandler;
typedef std::shared_ptr<UiHandler> UiHandlerPtr;

/***
 * Contains information on the current control state.  Eventually, its members should
 * be made analog instead of digital (well, the ones that can, at least).
 */
struct ControlState {
	// TODO: make these inputs analog, not digital
	bool motorOn;
	bool jump;
	bool brake;
	bool fire;
	bool weapon;
	bool lookBack;
	bool right;
	bool left;
};

class InputEventController {
	public:
		InputEventController(Util::OS::wnd_t mainWindow, UiHandlerPtr uiHandler);
		~InputEventController();

		// Typedef for the maps of hashes to controls
		typedef std::map<int, ControlActionPtr> ActionMap;

		// event handlers
		bool OnKeyPressed(const SDL_KeyboardEvent &arg);
		bool OnKeyReleased(const SDL_KeyboardEvent &arg);
		/*TODO
		bool mouseMoved(const MouseEvent &arg);
		bool mousePressed(const MouseEvent &arg, MouseButtonID id);
		bool mouseReleased(const MouseEvent &arg, MouseButtonID id);
		bool buttonPressed(const JoyStickEvent &arg, int button);
		bool buttonReleased(const JoyStickEvent &arg, int button);
		bool axisMoved(const JoyStickEvent &arg, int axis);
		bool povMoved(const JoyStickEvent &arg, int pov);
		*/

		void Poll();
		void HandleEvent(int hash, int value);

		/***
		 * This function tells the InputEventController to capture the next user input
		 * event and assign the action currently residing at 'oldhash' to the hash of
		 * the new input.  Behavior is undefined if there is no action assigned to the
		 * old hash, so don't screw it up!  This is meant to be called by the control
		 * assignment dialog box.
		 *
		 * @param oldhash Old hash.
		 * @param mapname String representing the name of the map.
		 */
		void CaptureNextInput(int oldhash, std::string mapname);

		/***
		 * This function indicates whether or not the controller is capturing an input.
		 * It can be used to check whether or not an input has been captured.
		 */
		bool IsCapturing();

		/***
		 * This function stops a capture.
		 */
		void StopCapture();

		/***
		 * This function assigns the next disabled hash to the current capture control.
		 * Nothing will be done if the InputEventController is not in capture mode.  This
		 * function will also disable capture mode.
		 */
		void DisableCaptureInput();

		/***
		 * Clears all of the active control bindings.  Does not delete the bindings
		 * but simply removes them from the active action map.
		 */
		void ClearActionMap();

		/***
		 * Add an action map into the current action map.  The available maps are
		 * referenced by string.  Maps include:
		 *
		 * "player1" ... "player4"
		 *
		 * @return false if the map is not found
		 */
		bool AddActionMap(std::string mapname);

		/***
		 * Return a vector containing the current maps.
		 */
		const std::vector<std::string>& GetActiveMaps();

		/***
		 * Return the map with the given key.
		 */
		ActionMap& GetActionMap(std::string key);

		/***
		 * Return a vector containing the names of all the available maps.
		 */
		std::vector<std::string> GetAvailableMaps();

		/***
		 * Update player mappings to point to correct MainCharacter objects, then
		 * add them to the active action map.  If NULL is passed as any of the pointers
		 * that map will not be added.
		 * The "console-toggle" map will also be added.
		 */
		void AddPlayerMaps(int numPlayers, MainCharacter::MainCharacter** mcs);

		/***
		 * Update pointers to Observer objects and add "Camera" map to the active
		 * action maps.
		 */
		void AddObserverMaps(Observer** obs, int numObs);

		/// Enable menu controls.
		void AddMenuMaps();

		/***
		 * Convert a hash into an internationalized string.
		 */
		std::string HashToString(int hash);

	private:
		template<class T>
		static void AssignAction(ActionMap &cmap, int hash, const T &action)
		{
			cmap[hash] = action;
			action->SetPrimaryTrigger(hash);
		}

	public:
		/// Set up menu controls.
		void LoadMenuMap();

		/***
		 * Set up controls for the console.
		 */
		void LoadConsoleMap();

		/***
		 * Update the pointer to the Console.
		 */
		void SetConsole(HoverScript::HighConsole* hc);

		/***
		 * Save the controller configuration to the Config object.
		 */
		void SaveConfig();

		/***
		 * Clear and reload entire configuration.
		 */
		void ReloadConfig();

		/***
		 * Load the configuration from the Config object.
		 */
		void LoadConfig();

	private:
		void InitInputManager(Util::OS::wnd_t mainWindow);

		// Auxiliary functions
		void RebindKey(std::string mapname, int oldhash, int newhash);

		// Hashing scheme (we have 32 bits but won't always use them):
		// disabled control
		// [000000000000000000][aaaaaaaaaaaa]
		//   a: next available disabled id
		// keyboard event
		// [00000000][00][000000][aaaaaaaa][00000000]
		//	 a: int keycode
		// mouse event
		// [00000000][01][00][aaaaaaaa][000000000000]: button press
		//   a: button id
		// [00000000][01][01][aaaa][bbbb][000000000000]: axis move
		//	 a: axis id
		//   b: direction
		// joystick event
		// [00000000][10][00][aaaaaaaa][bbbbbbbb][0000]: button press
		//   a: joystick id
		//   b: button id
		// [00000000][10][01][aaaaaaaa][bbbbbbbb][0000]: slider move
		//   a: joystick id
		//   b: slider id
		// [00000000][10][10][aaaaaaaa][bbbb][cccc][0000]: pov move
		//   a: joystick id
		//   b: direction
		//   c: pov id
		// [00000000][10][11][aaaaaaaa][bbbb][cccc][0000]: axis move
		//   a: joystick id
		//   b: axis id
		//   c: direction
		int GetNextAvailableDisabledHash();

	public:
		int HashKeyboardEvent(const SDL_Keycode& arg);
		/*TODO
		int HashMouseButtonEvent(const MouseEvent& arg, MouseButtonID id);
		int HashMouseAxisEvent(const MouseEvent& arg, int axis, int direction);
		int HashJoystickAxisEvent(const JoyStickEvent& arg, int axis, int direction);
		int HashJoystickSliderEvent(const JoyStickEvent& arg, int slider);
		int HashJoystickButtonEvent(const JoyStickEvent& arg, int button);
		int HashJoystickPovEvent(const JoyStickEvent& arg, int pov, int direction);
		*/

	private:
		/***
		 * We store several different action maps which we can choose from.
		 * They are referenced by string.  See ClearActionMap(), AddActionMap().
		 */
		ActionMap actionMap;
		std::vector<std::string> activeMaps;
		std::map<std::string, ActionMap> allActionMaps;

		UiHandlerPtr uiHandler;

		// now the input devices
		/*FIXME
		Keyboard *kbd;
		Mouse *mouse;
		int numJoys;
		JoyStick **joys;
		int *joyIds;
		*/

		int nextAvailableDisabledHash;

		bool captureNextInput;
		int  captureOldHash; /// stores the value of the hash we will be replacing when capturing input
		std::string captureMap; /// name of the map we are capturing for

	public:
		typedef std::shared_ptr<Action<voidSignal_t>> VoidActionPtr;
		struct actions_t {
			struct ui_t {
				ui_t();
				VoidActionPtr menuOk;
				VoidActionPtr menuCancel;
			} ui;
		} actions;
};

} // namespace Control
} // namespace Client
} // namespace HoverRace

#endif
