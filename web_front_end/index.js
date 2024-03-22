window.onload = async () => {
    const button = document.getElementById("butt");
    const status_box = document.getElementById("status");
    let state;
    button.addEventListener("click", async () =>
    {
        await fetch("/set_state", {
            method: "POST",
            body: state ? "0" : "1"
        });
        set_state(!state);
    });
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
    const resp = await fetch("/get_state");    
    set_state(Boolean(Number(await resp.text())));
    document.getElementsByTagName("main")[0].classList.add("fade-in");
}
