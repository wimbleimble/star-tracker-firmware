// State enum
const State = Object.freeze({
    STOPPED: 0,
    NORMAL: 1,
    BOOST: 2,
    REWIND: 3
});

// Lookup table for status field for different states.
const STATUS_TABLE = Object.freeze([
   "Device is currently stopped.", // STOPPED
   "Device is rotating.",          // NORMAL
   "Fast-forwarding...",           // BOOST
   "Rewinding..."                  // REWIND
]);

// Lookup table for state transitions for a given input.
const TRANSITION_TABLE = Object.freeze({
    play_stop: [
        State.NORMAL,
        State.STOPPED,
        State.STOPPED,
        State.STOPPED,
    ],
    ffwd: [
        State.BOOST,
        State.BOOST,
        State.NORMAL,
        State.BOOST
    ],
    rewind: [
        State.REWIND,
        State.REWIND,
        State.REWIND,
        State.NORMAL
    ]
});

// Update ui in accordance with specified state.
function update_ui(new_state, ui) {
    switch (new_state)
    {
        case State.STOPPED:
            ui.play_icon.classList.remove("hide");
            ui.stop_icon.classList.add("hide");
            break;
        case State.NORMAL:
        case State.BOOST:
        case State.REWIND:
            ui.stop_icon.classList.remove("hide");
            ui.play_icon.classList.add("hide");
            break;
        default:
            throw new Error("Invalid state provided");
    }
    ui.status_box.innerHTML = STATUS_TABLE[new_state];
}

// Transition to new state.
async function state_transition(new_state, ui) {

    await fetch("/set_state", {
        method: "POST",
        body: String(new_state)
    });

    update_ui(new_state, ui);

    return new_state;
}

// Sync state with server.
async function sync_state(ui) {
    const state_resp = await fetch("/get_state");
    const step_resp = await fetch("/get_step_mode");
    const state = Number(await state_resp.text())

    ui.step_mode_sel.selectedIndex = Math.log2(Number(await step_resp.text()));
    update_ui(state, ui);

    return state;
}

window.onload = async () => {
    const ui = {
        play_stop: document.getElementById("play_stop"),
        play_icon : document.getElementById("play"),
        stop_icon : document.getElementById("stop"),
        rewind: document.getElementById("rewind"),
        ffwd: document.getElementById("ffwd"),
        status_box: document.getElementById("status"),
        step_mode_sel: document.getElementById("step_mode"),
    };

    let state = await sync_state(ui);

    ui.play_stop.addEventListener("click", async () =>
        state = await state_transition(TRANSITION_TABLE.play_stop[state], ui));

    ui.rewind.addEventListener("click", async () =>
        state = await state_transition(TRANSITION_TABLE.rewind[state], ui));

    ui.ffwd.addEventListener("click", async () =>
        state = await state_transition(TRANSITION_TABLE.ffwd[state], ui));

    ui.step_mode_sel.addEventListener("change", async () => {
        await fetch("/set_step_mode", {
            method: "POST",
            body: ui.step_mode_sel.value
        });
    }
    );

    document.getElementsByTagName("main")[0].classList.add("fade-in");
}
