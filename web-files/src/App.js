import React from "react";
import Login from "./components/Login.js";
import Home from "./components/Home.js";
import Register from "./components/Register.js";
import { BrowserRouter, Routes, Route } from "react-router-dom";

function setToken(userToken) {
  sessionStorage.setItem("token", JSON.stringify(userToken));
}

function getToken() {
  const tokenString = sessionStorage.getItem("token");
  const userToken = JSON.parse(tokenString);
  return userToken?.token;
}

function App() {
  const token = getToken();

  return (
    <div className="App">
      <BrowserRouter>
        <Routes>
          <Route path="/" element={<Login setToken={setToken} />} />
          <Route path="/home" element={<Home/>} />
          <Route path="/register" element={<Register setToken={setToken} />} />
        </Routes>
      </BrowserRouter>
    </div>
  );
}

export default App;
