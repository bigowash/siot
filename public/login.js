import { initializeApp } from "https://www.gstatic.com/firebasejs/10.6.0/firebase-app.js";
import {
  getAuth,
  signInWithEmailAndPassword,
} from "https://www.gstatic.com/firebasejs/10.6.0/firebase-auth.js";

const firebaseConfig = {
  apiKey: "AIzaSyBajN795iPI-_xAKalgH1MEzX74z6OImbM",
  authDomain: "siot-16d63.firebaseapp.com",
  databaseURL:
    "https://siot-16d63-default-rtdb.europe-west1.firebasedatabase.app",
  projectId: "siot-16d63",
  storageBucket: "siot-16d63.appspot.com",
  messagingSenderId: "280565145804",
  appId: "1:280565145804:web:d6d93dea9a1eba9936a269",
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const auth = getAuth(app);

const loginButton = document.getElementById("login-btn");
const emailInput = document.getElementById("email");
const passwordInput = document.getElementById("password");
const loginError = document.getElementById("login-error");

loginButton.addEventListener("click", () => {
  const email = emailInput.value;
  const password = passwordInput.value;

  console.log("first");

  signInWithEmailAndPassword(auth, email, password)
    .then((userCredential) => {
      // Handle successful login here
      console.log("Login successful:", userCredential.user);

      // Redirect to another page, e.g., the main page of your application
      window.location.href = "index.html"; // Adjust the redirection page as needed
    })
    .catch((error) => {
      // Handle Errors here.
      loginError.textContent = error.message;
      console.error("Login error:", error.message);
    });
});
