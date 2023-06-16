import React from "react";
import "../styles/AddFile.css";
import ipAddress from '../config';

function AddFile({ onFileAdded }) {
  const handleFileInputChange = (event) => {
    const file = event.target.files[0];
    onFileAdded(file);
    // Send the file to the server
    sendFileToServer(file);
  };

  const sendFileToServer = async (file) => {
    const formData = new FormData();
    formData.append("file", file);

    console.log("Data being sent to the server:", file); // Log the data being sent

    try {
      const response = await fetch(`${ipAddress}/upload`, {
        method: "POST",
        body: formData,
      });
      // Handle the response from the server
      if (response.ok) {
        console.log("File uploaded successfully!");
      } else {
        console.log("Failed to upload file!");
      }
    } catch (error) {
      console.log("Error occurred while uploading file:", error);
    }
  };

  return (
    <div>
      <input
        type="file"
        id="fileInput"
        className="hidden-input"
        onChange={handleFileInputChange}
        style={{ display: "none" }}
      />
      <label htmlFor="fileInput" className="main-button">
        Add File
      </label>
    </div>
  );
}

export default AddFile;
