const express = require("express");
const app = express();
const soap = require("soap");
const bodyParser = require("body-parser");

app.use(bodyParser.raw({ type: () => true, limit: "5mb" }));

// Sample user data (you would typically use a database to store user data)
const users = [
  { username: "admin", password: "admin" },
  { username: "asd", password: "asd" },
];

// SOAP service implementation
const soapService = {
  AuthService: {
    AuthServiceSoap: {
      Login: function (args) {
        const { username, password } = args;
        const user = users.find(
          (user) => user.username === username && user.password === password
        );

        if (user) {
          return { success: true, message: "Login successful!" };
        } else {
          return { success: false, message: "Invalid username or password" };
        }
      },
      Register: function (args) {
        const { username, password } = args;
        const existingUser = users.find((user) => user.username === username);

        if (existingUser) {
          return { success: false, message: "Username already exists" };
        }

        // Store the new user (you would typically save it to a database)
        users.push({ username, password });

        return { success: true, message: "Registration successful!" };
      },
    },
  },
};

// WSDL file content
const wsdl = `
<definitions xmlns="http://schemas.xmlsoap.org/wsdl/" xmlns:soap="http://schemas.xmlsoap.org/wsdl/soap/" targetNamespace="http://example.com/auth">
  <types>
    <schema xmlns="http://www.w3.org/2001/XMLSchema">
      <element name="LoginRequest">
        <complexType>
          <sequence>
            <element name="username" type="string" />
            <element name="password" type="string" />
          </sequence>
        </complexType>
      </element>
      <element name="LoginResponse">
        <complexType>
          <sequence>
            <element name="success" type="boolean" />
            <element name="message" type="string" />
          </sequence>
        </complexType>
      </element>
      <element name="RegisterRequest">
        <complexType>
          <sequence>
            <element name="username" type="string" />
            <element name="password" type="string" />
          </sequence>
        </complexType>
      </element>
      <element name="RegisterResponse">
        <complexType>
          <sequence>
            <element name="success" type="boolean" />
            <element name="message" type="string" />
          </sequence>
        </complexType>
      </element>
    </schema>
  </types>
  <message name="LoginRequestMessage">
    <part name="parameters" element="tns:LoginRequest" />
  </message>
  <message name="LoginResponseMessage">
    <part name="parameters" element="tns:LoginResponse" />
  </message>
  <message name="RegisterRequestMessage">
    <part name="parameters" element="tns:RegisterRequest" />
  </message>
  <message name="RegisterResponseMessage">
    <part name="parameters" element="tns:RegisterResponse" />
  </message>
  <portType name="AuthServicePortType">
    <operation name="Login">
      <input message="tns:LoginRequestMessage" />
      <output message="tns:LoginResponseMessage" />
    </operation>
    <operation name="Register">
      <input message="tns:RegisterRequestMessage" />
      <output message="tns:RegisterResponseMessage" />
    </operation>
  </portType>
  <binding name="AuthServiceBinding" type="tns:AuthServicePortType">
    <soap:binding style="rpc" transport="http://schemas.xmlsoap.org/soap/http" />
    <operation name="Login">
      <soap:operation soapAction="http://example.com/auth/Login" />
      <input>
        <soap:body use="encoded" encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" namespace="urn:examples:auth" />
      </input>
      <output>
        <soap:body use="encoded" encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" namespace="urn:examples:auth" />
      </output>
    </operation>
    <operation name="Register">
      <soap:operation soapAction="http://example.com/auth/Register" />
      <input>
        <soap:body use="encoded" encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" namespace="urn:examples:auth" />
      </input>
      <output>
        <soap:body use="encoded" encodingStyle="http://schemas.xmlsoap.org/soap/encoding/" namespace="urn:examples:auth" />
      </output>
    </operation>
  </binding>
  <service name="AuthService">
    <port name="AuthServiceSoap" binding="tns:AuthServiceBinding">
      <soap:address location="http://localhost:5000/soap-endpoint" />
    </port>
  </service>
</definitions>
`;

// Serve the WSDL file
app.get("/soap-endpoint?wsdl", (req, res) => {
  res.set("Content-Type", "application/xml");
  res.send(wsdl);
});

// Create the SOAP server
const soapServer = soap.listen(app, "/soap-endpoint", soapService, wsdl);

// Start the server
const port = process.env.PORT || 5001;
app.listen(port, () => {
  console.log(`SOAP endpoint is running on port ${port}`);
});
