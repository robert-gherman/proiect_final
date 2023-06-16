import React, { useState } from "react";
import AddFile from "./AddFile";
import SelectedFile from "./SelectedFile";
import "../styles/FileWindow.css";
import ipAddress from "../config";

function FileWindow() {
  const [selectedFiles, setSelectedFiles] = useState([]);

  const handleFileAdded = (file) => {
    setSelectedFiles((prevSelectedFiles) => [...prevSelectedFiles, file]);
    requestSelectedFileFromServer(file);
  };

  const requestSelectedFileFromServer = async (file) => {
  try {
    const response = await fetch(`${ipAddress}/getfile?filename=${encodeURIComponent(file.name)}`, {
      method: "GET",
      headers: {
        "Content-Type": "application/json",
      },
    });

    // Handle the response from the server
    if (response.ok) {
      // const fileData = await response.arrayBuffer();
      // console.log("Selected file data:", fileData);
      // Process the file data as per your requirement
    } else {
      // console.log("Failed to get the selected file!");
    }
  } catch (error) {
    // console.log("Error occurred while getting the selected file:", error);
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
