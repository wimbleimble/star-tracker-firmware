window.onload = async () => {
    const button = document.getElementById("butt");
    const status_box = document.getElementById("status");
    const step_mode_sel = document.getElementById("step_mode");
    let state;
    button.addEventListener("click", async () =>
    {
        await fetch("/set_state", {
            method: "POST",
            body: state ? "0" : "1"
        });
        set_state(!state);
    });
    step_mode_sel.addEventListener("change", async () =>
    {
        await fetch("/set_step_mode", {
            method: "POST",
            body: step_mode_sel.value
        });
    }
    );
    function set_state(new_state)
    {
        if(new_state == state)
            return;

        if(new_state)
        {
            button.innerHTML = "Stop";
            status_box.innerHTML = "Device is rotating.";
        }
        else
        {
            button.innerHTML = "Start";
            status_box.innerHTML = "Device is currently stopped.";
        }
        state = new_state;
    }
    const state_resp = await fetch("/get_state");    
    const step_resp = await fetch("/get_step_mode");    
    set_state(Boolean(Number(await state_resp.text())));
    step_mode_sel.selectedIndex = Math.log2(Number(await step_resp.text()));
    document.getElementsByTagName("main")[0].classList.add("fade-in");
}
