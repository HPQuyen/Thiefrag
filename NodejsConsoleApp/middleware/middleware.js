

module.exports = {
    CheckAuthorized(req, res, next) {
        console.log("middleware check auth call");
        var isAuthorized = false;
        if (req.session.isAuthorized === "authorized") {
            isAuthorized = true;
        }
        res.locals.isAuthorized = isAuthorized;
        return next();
    },
    InitCart(req, res, next) {
        if (req.session.cart === undefined) {
            req.session.cart = [];
        }
        next();
    }
}