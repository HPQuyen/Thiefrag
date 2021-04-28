const express = require('express');
const router = express.Router();
const firebase = require("firebase/app");
const emailService = require('../controllers/emailService.route');
require("firebase/database");

// My web app's Firebase configuration
var firebaseConfig = {
    apiKey: "AIzaSyAextH1u5uq3LlcZ-5SoMdhuPuwx1jcl6E",
    authDomain: "thiefrag-65129.firebaseapp.com",
    databaseURL: "https://thiefrag-65129-default-rtdb.firebaseio.com",
    projectId: "thiefrag-65129",
    storageBucket: "thiefrag-65129.appspot.com",
    messagingSenderId: "72980298760",
    appId: "1:72980298760:web:37ddd00445e53b487925bd"
};

// Initialize Firebase
firebase.initializeApp(firebaseConfig);


function checkAuthMiddlware(req, res, next) {
    // Middleware check auth protect this route
    if (!res.locals.isAuthorized) {
        return res.redirect('/login');
    }
    next();
}

router.get('/', checkAuthMiddlware, (req, res) => {
    res.render('vwHome/index', { 
        title: 'Trang chủ' 
    });
})

router.get('/login', (req, res) => {
    if (!res.locals.isAuthorized) {
        return res.render('vwLogin/index', {
            title: 'Đăng nhập',
            layout: false
        });
    }
    return res.redirect('/');
})

router.get('/change-password', checkAuthMiddlware, (req, res) => {
    res.render('vwChangePassword/index', { 
        title: 'Đổi mật khẩu' 
    });
})

router.get('/settings', checkAuthMiddlware, (req, res) => {
    const dbRef = firebase.database().ref();
    dbRef.child("settings").get().then((snapshot) => {
        if (snapshot.exists()) {
            res.locals.IODevice = snapshot.val().IODevice === "on";
            res.locals.IOAutomatic = snapshot.val().IOAutomatic === "on";
            console.log(snapshot.val());
            console.log(res.locals);
            res.render('vwSettings/index', {
                title: 'Thiết lập'
            });
        } else {
            console.log("No data available");
        }
    }).catch((error) => {
        console.error(error);
    });
})

router.post('/settings', checkAuthMiddlware, (req, res) => {
    try {
        const dbRef = firebase.database().ref();
        var updates = {};
        if (req.body.IODevice)
            updates['/settings/IODevice'] = req.body.IODevice;
        if (req.body.IOAutomatic)
            updates['/settings/IOAutomatic'] = req.body.IOAutomatic;
        dbRef.update(updates);
        res.json(null);
    } catch (e) {
        res.json(e.message);
    }
})

router.post('/login', (req, res) => {
    const dbRef = firebase.database().ref();
    dbRef.child("account").get().then((snapshot) => {
        if (snapshot.exists()) {
            var username = snapshot.val().username;
            var password = snapshot.val().password;
            if (req.body.username === username && req.body.password === password) {
                req.session.isAuthorized = "authorized";
                return res.json(null);
            }
            return res.json("Username or password is incorrect");
        } else {
            console.log("No data available");
        }
    }).catch((error) => {
        console.error(error);
    });
})

router.get('/logout', checkAuthMiddlware, (req, res) => {
    req.session.isAuthorized = undefined;
    res.redirect('/login');
})

router.post('/changepassword', checkAuthMiddlware, (req, res) => {
    const dbRef = firebase.database().ref();
    dbRef.child("account").get().then((snapshot) => {
        if (snapshot.exists()) {
            var password = snapshot.val().password;
            if (req.body.oldPassword !== password) {
                return res.json("Password is incorrect");
            }
            var updates = {};
            updates['/account/password'] = req.body.newPassword;
            firebase.database().ref().update(updates);
            res.json(null);
        } else {
            console.log("No data available");
        }
    }).catch((error) => {
        console.error(error);
    });

})
//const API_KEY = "5E884898DA28047151D0E56F8DC6292773603D0D6AABBDD62A11EF721D1542D8";

router.post('/mailservice', (req, res) => {
    console.log(req.headers);
    emailService.sendConfirmationEmail({
        email: "phuquyen1504@gmail.com"
    }, (error, data) => {
        if (error) {
            return res.sendStatus(417);
        }
        return res.sendStatus(200);
    });
})

module.exports = router;