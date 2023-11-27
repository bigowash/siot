// Import Firebase modules
import { initializeApp } from "https://www.gstatic.com/firebasejs/10.6.0/firebase-app.js";
import { getDatabase, ref, onValue } from "https://www.gstatic.com/firebasejs/10.6.0/firebase-database.js";

// Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyBajN795iPI-_xAKalgH1MEzX74z6OImbM",
  authDomain: "siot-16d63.firebaseapp.com",
  databaseURL: "https://siot-16d63-default-rtdb.europe-west1.firebasedatabase.app",
  projectId: "siot-16d63",
  storageBucket: "siot-16d63.appspot.com",
  messagingSenderId: "280565145804",
  appId: "1:280565145804:web:d6d93dea9a1eba9936a269"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);
const database = getDatabase(app);

let countdownInterval; // Global variable for the countdown interval
function startCountdown(duration, displayElement) {
  let timer = duration, minutes, seconds;
  countdownInterval = setInterval(() => {
    minutes = parseInt(timer / 60, 10);
    seconds = parseInt(timer % 60, 10);

    minutes = minutes < 10 ? "0" + minutes : minutes;
    seconds = seconds < 10 ? "0" + seconds : seconds;

    displayElement.textContent = minutes + ":" + seconds;

    if (--timer < 0) {
      clearInterval(countdownInterval);
      displayElement.textContent = 'Time is up!';
    }
  }, 1000);
}

function createSeatBox(seatNumber) {
  // Create elements for seat box
  const seatBox = document.createElement('div');
  seatBox.id = `seat${seatNumber}`;
  seatBox.className = 'seatBox';

  // Add seat information elements
  seatBox.innerHTML = `
<h2>Seat ${seatNumber}</h2>
<p>Seat Available: <span id="seatAvailable${seatNumber}"></span></p>
<p>Card Number: <span id="cardNumber${seatNumber}"></span></p>
<p>Timer: <span id="timer${seatNumber}"></span></p>
<button id="historyButton${seatNumber}">See History</button>
<button id="analysisButton${seatNumber}">See Analysis</button>
`;

  document.getElementById('seatsContainer').appendChild(seatBox);

  // Fetch and display data for this seat
  fetchSeatData(seatNumber);

  // Attach event listeners
  document.getElementById(`historyButton${seatNumber}`).addEventListener('click', () => showHistory(seatNumber));
  document.getElementById(`analysisButton${seatNumber}`).addEventListener('click', () => showAnalysis(seatNumber));
}

function showAnalysis(seatNumber) {
  const seatRef = ref(database, `Seats/${seatNumber}`);
  onValue(seatRef, (snapshot) => {
    const data = snapshot.val();
    performAnalysis(data);
  }, { onlyOnce: true });  // Fetch data only once when the button is clicked
}

function performAnalysis(data) {
  // Extract events and sort by timestamp
  const events = Object.values(data).filter(event => event.event && event.timestamp);
  events.sort((a, b) => a.timestamp - b.timestamp);

  let analysisHtml = '<h2>Seat Analysis</h2>';

  // Prepare to calculate card usage durations
  const cardDurations = {}; // Store total duration for each card
  const cardAppearanceCounts = {}; // Store appearance count for each card

  let lastCardUID = null;
  let lastCardTimestamp = null;

  events.forEach(event => {
      if (event.event === "Card Placed") {
          lastCardUID = event.cardUID;
          lastCardTimestamp = event.timestamp;
      } else if (event.event === "Card Removed" && lastCardUID) {
          const duration = (event.timestamp - lastCardTimestamp) / 60000; // Convert ms to minutes
          cardDurations[lastCardUID] = (cardDurations[lastCardUID] || 0) + duration;
          cardAppearanceCounts[lastCardUID] = (cardAppearanceCounts[lastCardUID] || 0) + 1;
          lastCardUID = null;
      }
  });

  // Display Card Usage Frequencies
  analysisHtml += `<p>Card Usage Frequencies:</p><ul>`;
  for (const [card, frequency] of Object.entries(cardAppearanceCounts)) {
      analysisHtml += `<li>${card}: ${frequency} times</li>`;
  }
  analysisHtml += `</ul>`;

  // Display Durations for Each Card
  analysisHtml += `<p>Card Durations (in minutes):</p><ul>`;
  let totalDuration = 0;
  let totalCount = 0;
  for (const [card, duration] of Object.entries(cardDurations)) {
      analysisHtml += `<li>${card}: ${duration.toFixed(2)} minutes</li>`;
      totalDuration += duration;
      totalCount += cardAppearanceCounts[card];
  }
  analysisHtml += `</ul>`;

  // Calculate and Display Average Duration
  const averageDuration = totalCount > 0 ? totalDuration / totalCount : 0;
  analysisHtml += `<p>Average Duration: ${averageDuration.toFixed(2)} minutes</p>`;

  // Display in the modal or a designated section
  document.getElementById('historyData').innerHTML = analysisHtml;
  document.getElementById('historyModal').style.display = 'block';
}


function fetchAllSeats() {
  const seatsRef = ref(database, 'Seats');

  onValue(seatsRef, (snapshot) => {
    const seatsData = snapshot.val();
    const seatNumbers = Object.keys(seatsData);

    seatNumbers.forEach(seatNumber => createSeatBox(seatNumber));
  }, { onlyOnce: true });
}

function fetchSeatData(seatNumber) {
  const seatRef = ref(database, `Seats/${seatNumber}`);

  onValue(seatRef, (snapshot) => {
    const data = snapshot.val();
    console.log(`Data received for seat ${seatNumber}:`, data);

    const isSeatAvailable = data.lightState === "Green";
    console.log(`Seat ${seatNumber} availability: ${isSeatAvailable ? 'Available' : 'Occupied'}`);

    document.getElementById(`seatAvailable${seatNumber}`).textContent = isSeatAvailable ? 'Yes' : 'No';
    document.getElementById(`seat${seatNumber}`).style.backgroundColor = isSeatAvailable ? 'green' : 'red';

    if (isSeatAvailable) {
      resetTimer(seatNumber);
      document.getElementById(`cardNumber${seatNumber}`).textContent = 'NA';
    } else {
      const events = Object.values(data).filter(event => event.event && event.timestamp);
      events.sort((a, b) => b.timestamp - a.timestamp);

      const recentCardPlacedEvent = events.find(event => event.event === "Card Placed");
      const recentCardRemovedEvent = events.find(event => event.event === "Card Removed");

      if (recentCardPlacedEvent && recentCardRemovedEvent && recentCardPlacedEvent.timestamp > recentCardRemovedEvent.timestamp) {
        resetTimer(seatNumber);
        document.getElementById(`cardNumber${seatNumber}`).textContent = recentCardPlacedEvent.cardUID || 'NA';
      } else if (recentCardRemovedEvent) {
        const currentTime = Date.now();
        const timeSinceCardRemoved = currentTime - recentCardRemovedEvent.timestamp;
        const fifteenMinutes = 15 * 60; // 15 minutes in seconds
        const timeRemaining = fifteenMinutes - Math.floor(timeSinceCardRemoved / 1000);

        if (timeRemaining > 0) {
          startCountdown(timeRemaining, document.getElementById(`timer${seatNumber}`));
        } else {
          document.getElementById(`timer${seatNumber}`).textContent = 'Time is up!';
        }
      } else {
        resetTimer(seatNumber);
      }
    }
  });
}

function resetTimer(seatNumber) {
  if (countdownInterval) {
    clearInterval(countdownInterval);
  }
  document.getElementById(`timer${seatNumber}`).textContent = 'NA';
}

function setupEventListeners() {
  const historyButton = document.getElementById('historyButton');
  if (historyButton) {
    historyButton.addEventListener('click', showHistory);
  } else {
    console.error('History button not found');
  }

  const modal = document.getElementById('historyModal');
  const span = document.getElementsByClassName("close")[0];

  if (modal && span) {
    // When the user clicks on <span> (x), close the modal
    span.onclick = function () {
      modal.style.display = "none";
    }

    // When the user clicks anywhere outside of the modal, close it
    window.onclick = function (event) {
      if (event.target == modal) {
        modal.style.display = "none";
      }
    }
  } else {
    console.error('Modal or close button not found');
  }
}

function showHistory(seatNumber) {
  const seatRef = ref(database, `Seats/${seatNumber}`);
  onValue(seatRef, (snapshot) => {
    const data = snapshot.val();
    displayHistory(data);
  }, { onlyOnce: true });  // Fetch data only once when the button is clicked
}

function displayHistory(data) {
  let historyHtml = '<h2>Seat History</h2>';
  const events = Object.values(data).filter(event => event.event && event.timestamp);

  events.sort((a, b) => a.timestamp - b.timestamp);

  events.forEach(event => {
    const dateTime = new Date(event.timestamp).toLocaleString();
    historyHtml += `<p>${event.event} at ${dateTime}</p>`;
  });

  // Display in the modal
  document.getElementById('historyData').innerHTML = historyHtml;
  document.getElementById('historyModal').style.display = 'block';
}

// Get the modal
const modal = document.getElementById('historyModal');

// Get the <span> element that closes the modal
const span = document.getElementsByClassName("close")[0];

// When the user clicks on <span> (x), close the modal
span.onclick = function () {
  modal.style.display = "none";
}

// When the user clicks anywhere outside of the modal, close it
window.onclick = function (event) {
  if (event.target == modal) {
    modal.style.display = "none";
  }
}

// Set up event listeners after DOM content is loaded
document.addEventListener('DOMContentLoaded', () => {
  setupEventListeners();
  fetchAllSeats();

});