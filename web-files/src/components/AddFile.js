import React from "react";
import "../styles/AddFile.css";

function AddFile({ onFileAdded }) {
  const handleFileInputChange = (event) => {
    const file = event.target.files[0];
    // Pass the selected file to the parent component
    onFileAdded(file);
    // Send the file to the server
    sendFileToServer(file);
  };

  const sendFileToServer = async (file) => {
    const formData = new FormData();
    formData.append("file", file);

    try {
      const response = await fetch("http://localhost:5000/upload", {
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
