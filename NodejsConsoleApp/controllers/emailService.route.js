const nodemailer = require("nodemailer");

module.exports = {
    sendConfirmationEmail(user, callback) {
        const transporter = nodemailer.createTransport({
            service: 'gmail',
            secure: true,
            auth: {
                user: "yanghoco3002@gmail.com",
                pass: "9:00*pm*15/04"
            },
            tls: {
                // do not fail on invalid certs
                rejectUnauthorized: false
            }
        });
        let mailOptions = {
            from: "Thiefrag-SMTP",
            to: user.email,
            subject: 'Thiefrage Device ***WARNING!!!***',
            text: 'Hi',
            html: {
                path: process.cwd() + '/views/mail_template.html'
            }
        };

        transporter.sendMail(mailOptions, function (err, data) {
            callback(err, data);
        });
    }
};