import React, { useState } from "react";
import { Link, useNavigate } from "react-router-dom";
import "../styles/Login.css";
import PropTypes from "prop-types";

async function loginUser(credentials) {
  try {
    const response = await fetch("http://localhost:5000/api/login", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify(credentials),
    });

    console.log(response); // Log the response object

    if (!response.ok) {
      throw new Error("Login failed");
    }

    const data = await response.text();
    console.log("Response:", data); // Log the response data

    console.log("Character at position 31:", data.charCodeAt(31)); // Log the character at position 31

    return JSON.parse(data);
  } catch (error) {
    console.error("Error:", error);
    throw error;
  }
}

export default function Login() {
  const [username, setUserName] = useState("");
  const [password, setPassword] = useState("");
  const navigate = useNavigate();

  const handleSubmit = async (e) => {
    e.preventDefault();
    try {
      const response = await loginUser({
        username,
        password,
      });
      console.log("DA");
      if (response && response.message === "Login successful!") {
        console.log("Login successful.");
        navigate("/home");
      } else {
        console.log("Login failed.");
      }
    } catch (error) {
      console.error("Error:", "NOPE");
    }
  };

  return (
    <div className="login-page" style={{ backgroundColor: "#1c1c1c" }}>
      <div className="login-container">
        <form className="form" onSubmit={handleSubmit}>
          <div className="mb-4">
            <label className="label">Username</label>
            <input
              className="input"
              name="username"
              placeholder="Type username..."
              value={username}
              onChange={(e) => setUserName(e.target.value)}
            />
          </div>
          <div className="mb-8">
            <label className="label">Password</label>
            <input
              className="input"
              type="password"
              name="password"
              placeholder="Password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
            />
          </div>
          <div className="flex justify-between flex-col">
            <button type="submit" className="submit">
              Login
            </button>
            <div className="buttons-container">
              <Link to="/register" className="link">
                New here? SIGN UP
              </Link>
            </div>
          </div>
        </form>
      </div>
    </div>
  );
}

Login.propTypes = {
  setToken: PropTypes.func.isRequired,
};
