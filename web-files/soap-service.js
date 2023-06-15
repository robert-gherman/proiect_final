const express = require('express');
const bodyParser = require('body-parser');
const cors = require('cors');
const soap = require('soap');
const fs = require('fs');
// Create an instance of Express app
const app = express().use(cors());
app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());



// Define the SOAP service implementation
const service = {
  examples: {
    mysoap: {
      login: function(args, callback) {
        console.log('Received login request:', args);

        // Implement your login logic here
        const { username, password } = args;

        // Perform authentication checks
        if (username === 'admin' && password === 'admin') {
          callback(null, { success: true, message: 'Login successful!' });
        } else {
          callback({ message: 'Invalid username or password' });
        }
      },

      register: function(args, callback) {
        // Implement your registration logic here
        const { username, password } = args;

        // Perform registration and return appropriate response
        callback(null, { success: true, message: 'Registration successful!' });
      }
    }
  }
};

// Create the SOAP server
const xml = require('fs').readFileSync('soap-service.wsdl', 'utf8');
const server = soap.listen(app, '/soap-endpoint', service, xml);

const PORT = process.env.PORT || 5001;
app.listen(PORT, function() {
  console.log('SOAP server running at http://localhost:' + PORT + '/soap-endpoint?wsdl');
});

function getSoapEndpointUrl() {
  return 'http://localhost:5001/soap-endpoint?wsdl';
}

module.exports = {
  getSoapEndpointUrl,
};
