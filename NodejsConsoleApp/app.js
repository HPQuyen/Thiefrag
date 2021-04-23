// Support framework modules
const express = require('express');
const morgan = require('morgan');
const exphbs = require('express-handlebars');
const express_handlebars_sections = require('express-handlebars-sections');
const path = require('path');
const session = require('express-session');
const ip = require("ip");

// Self-Definition modules
const middleware = require('./middleware/middleware');

// App-using declaration
const app = express();

app.set('trust proxy', 1) // trust first proxy

app.use(session({
    secret: 'PRIVATE_KEY',
    resave: false,
    saveUninitialized: true
}))

app.use(morgan('dev'));

app.use(express.urlencoded({
    extended: true
}));

const hbs = exphbs.create({
    defaultLayout: 'main',
    extname: '.hbs',
    layoutsDir: path.join(__dirname, '/views/layouts/'),
    partialsDir: path.join(__dirname, '/views/partials/'),
    helpers: {
        section: express_handlebars_sections(),
    }
})

app.engine('hbs', hbs.engine);

// Program configurations
app.set('view engine', 'hbs');

// Middleware
app.use(middleware.CheckAuthorized);

// Static declaration
app.use('/views/assets', express.static('views/assets'));

// Controllers declaration
try {
    app.use('/', require('./controllers/homepage.route'));

} catch (e) {
    console.log(e);
}

// App start listening
app.listen(80, ip.address(), function () {
    console.log(`App is listening at http://${ip.address()}:${80}`);
})