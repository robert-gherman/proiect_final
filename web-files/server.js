// const express = require('express');
// const app = express();
// const cors = require('cors');
// const soap = require('soap');
// const bodyParser = require('body-parser');
// const soapService = require('./soap-service');

// const corsOptions = {
//   origin: 'http://localhost:3000', // Replace with your client domain
//   methods: ['GET', 'POST'],
//   allowedHeaders: ['Content-Type', 'SOAPAction'],
// };

// app.use(cors(corsOptions));
// app.use(bodyParser.text({ type: 'text/xml' })); // Add this line

// const API_PORT = process.env.PORT || 5000;

// app.post('/soap-endpoint', async (req, res) => {
//   const { username, password } = req.body;
//   console.log('Received login request:', { username, password });
//   // Create a SOAP client
//   const url = soapService.getSoapEndpointUrl(); // Use the function to retrieve the SOAP endpoint URL
//   soap.createClient(url, (err, client) => {
//     if (err) {
//       res.status(500).json({ message: err.message });
//       return;
//     }
    
//     // Call the login SOAP method
//     client.login({ username, password }, (err, result) => {
//       console.log('SOAP response:', err, result);
//       if (err) {
//         res.status(500).json({ message: err.message });
//         return;
//       }

//       // Process the SOAP response
//       if (result.success) {
//         res.status(200).json({ message: 'Login successful!' });
//       } else {
//         res.status(401).json({ message: 'Invalid username or password' });
//       }
//     });
//   });
// });

// app.listen(API_PORT, () => console.log(`Server is running on port ${API_PORT}`));