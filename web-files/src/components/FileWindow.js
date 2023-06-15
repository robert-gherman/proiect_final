import React, { useState } from "react";
import AddFile from "./AddFile";
import SelectedFile from "./SelectedFile";
import "../styles/FileWindow.css"

function FileWindow() {
  const [selectedFiles, setSelectedFiles] = useState([]);

  const handleFileAdded = (file) => {
    setSelectedFiles((prevSelectedFiles) => [...prevSelectedFiles, file]);
    // Request the selected file from the server
    requestSelectedFileFromServer(file);
  };

  const requestSelectedFileFromServer = async (file) => {
    try {
      const response = await fetch("http://localhost:5000/getfile", {
        method: "POST",
        body: JSON.stringify({ filename: file.name }),
        headers: {
          "Content-Type": "application/json",
        },
      });
      // Handle the response from the server
      if (response.ok) {
        const fileData = await response.json();
        console.log("Selected file data:", fileData);
      } else {
        console.log("Failed to get the selected file!");
      }
    } catch (error) {
      console.log("Error occurred while getting the selected file:", error);
    }
  };

  return (
    <div className="filewindow">
      <AddFile onFileAdded={handleFileAdded} />
      <div className="FileWindow-container">
        {selectedFiles.length > 0 ? (
          <>
            <h3>Selected File:</h3>
            {selectedFiles.map((file, index) => (
              <SelectedFile key={index} file={file} />
            ))}
          </>
        ) : (
          <p>No file selected.</p>
        )}
      </div>
    </div>
  );
}

export default FileWindow;
