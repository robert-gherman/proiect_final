import React, { useState } from "react";

function SelectedFile({ file }) {
  const [isSelected, setIsSelected] = useState(false);
    
  const handleFileClick = () => {
    setIsSelected(!isSelected);
  };

  const handleSaveFileClick = () => {
  if (isSelected) {
    const link = document.createElement("a");
    link.href = URL.createObjectURL(file);
    link.download = file.name;

    link.style.display = "none";
    document.body.appendChild(link);

    link.click();

    document.body.removeChild(link);
  } else {
    const fileData = new Blob([file], { type: file.type });
    const link = document.createElement("a");
    link.href = URL.createObjectURL(fileData);
    link.download = file.name; // Set the desired file name here

    link.style.display = "none";
    link.setAttribute("download", "");
    link.setAttribute("target", "_blank");
    link.setAttribute("rel", "noopener noreferrer");
    link.click();

    URL.revokeObjectURL(link.href);
  }
};

  const selectedFileStyle = {
    backgroundColor: isSelected ? "rgb(216, 92, 10)" : "transparent",
    padding: "5px",
    borderRadius: "5px",
    cursor: "pointer",
  };

  return (
    <div style={selectedFileStyle} onClick={handleFileClick}>
      {file && <p>{file.name}</p>}
      <button onClick={handleSaveFileClick}>Save File</button>
    </div>
  );
}

export default SelectedFile;
