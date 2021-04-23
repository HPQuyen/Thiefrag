function DisplayError(message) {
    swal(message, {
        icon: "error",
        buttons: "OK",
    });
}

function SignIn() {
    if (jQuery("#inputEmail").val().length === 0 || jQuery("#inputPassword").val().length === 0) {
        return;
    }
    event.preventDefault();
    const username = jQuery("#inputEmail").val();
    const password = jQuery("#inputPassword").val();
    $.post('/login', {
        username: username,
        password: password
    }, function (err, data) {
        if (err) {
            console.log("Error");
            DisplayError(err);
            submitSuccess = false;
        } else {
            var url = "/";
            location.replace(url);
        }
    });
    submitSuccess = true;
}

var submitSuccess = false;

function KeyPressSubmit() {
    if (event.key === "Enter" && !submitSuccess) {
        SignIn();
    }
}