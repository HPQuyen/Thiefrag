const express = require('express');
const router = express.Router();

var username = "admin";
var password = "admin";

function checkAuthMiddlware(req, res, next) {
    // Middleware check auth protect this route
    if (!res.locals.isAuthorized) {
        return res.redirect('/login');
    }
    next();
}

router.get('/', checkAuthMiddlware, (req, res) => {
    res.render('home', {
        layout: false
    });
})

router.get('/login', (req, res) => {
    if (!res.locals.isAuthorized) {
        return res.render('index', {
            layout: false
        });
    }
    return res.redirect('/');
})
router.post('/login', (req, res) => {
    if (req.body.username === username && req.body.password === password) {
        req.session.isAuthorized = "authorized";
        return res.json(null);
    }
    return res.json("Username or password is incorrect");
})
router.post('/logout', (req, res) => {
    req.session.isAuthorized = undefined;
    res.redirect('/auth/login');
})
router.post('/changepassword', checkAuthMiddlware ,(req, res) => {
    if (req.body.oldPassword !== password) {
        return res.json("Password is incorrect");
    }
    password = req.body.newPassword;
    res.json(null);
})


module.exports = router;