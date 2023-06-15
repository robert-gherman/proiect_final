import React, { useState, useEffect } from "react";
import { Link, useNavigate } from "react-router-dom";
import ipAddress from '../config';
import "../styles/Login.css";

async function registerUser(credentials) {
  return fetch(`${ipAddress}/api/register`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify(credentials),
  })
    .then((response) => {
      if (!response.ok) {
        throw new Error("Network response was not ok");
      }
      return response.json();
    })
    .catch((error) => {
      console.error("Error:", error);
    });
}

export default function Register() {
  const [username, setUserName] = useState();
  const [password, setPassword] = useState();
  const [confirmPassword, setConfirmPassword] = useState();
  const [passwordsMatch, setPasswordsMatch] = useState(true);
  const [formSubmitted, setFormSubmitted] = useState(false);
  const navigate = useNavigate();

  useEffect(() => {
    if (formSubmitted && passwordsMatch) {
      navigate("/login");
    }
  }, [formSubmitted, passwordsMatch, navigate]);

  const handleSubmit = async (e) => {
    e.preventDefault();
    if (password !== confirmPassword) {
      setPasswordsMatch(false);
      return;
    }

    setPasswordsMatch(true);
    setFormSubmitted(true);

    await registerUser({
      username,
      password,
    });
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
              onChange={(e) => setPassword(e.target.value)}
            />
          </div>
          <div className="mb-8">
            <label className="label">Confirm Password</label>
            <input
              className="input"
              type="password"
              name="confirmPassword"
              placeholder="Confirm Password"
              onChange={(e) => setConfirmPassword(e.target.value)}
            />
          </div>
          {!passwordsMatch && (
            <div className="mb-4 text-red-500" style={{ color: "red" }}>
              Passwords do not match.
            </div>
          )}
          <div className="flex justify-between flex-col">
            <button type="submit" className="submit">
              Register
            </button>
            <div className="buttons-container">
              <Link to="/" className="link">
                Already have an account?
              </Link>
            </div>
          </div>
        </form>
      </div>
    </div>
  );
}
