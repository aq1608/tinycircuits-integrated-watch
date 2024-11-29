const express = require('express');
const { google } = require('googleapis');

// OAuth2 credentials (replace with your own)
const CLIENT_ID = '116505421886-b9tbkpm7f5b2esobbcsjuo0h6ghistn3.apps.googleusercontent.com';
const CLIENT_SECRET = 'Change to your own';
const REDIRECT_URI = 'http://localhost:3000/oauth2callback';
const REFRESH_TOKEN = '1//0gplRQpmlQv8OCgYIARAAGBASNwF-L9Ir9-BZj2T-Sqpzc1aTjuKLmITf9pcbY1dugJFBtUkJTrHZDuXDywF_PLy1iN9OSqx_-GQ';

// Initialize the OAuth2 client
const oauth2Client = new google.auth.OAuth2(CLIENT_ID, CLIENT_SECRET, REDIRECT_URI);
oauth2Client.setCredentials({ refresh_token: REFRESH_TOKEN });

// Initialize Google Calendar API
const calendar = google.calendar({ version: 'v3', auth: oauth2Client });

// Create Express app
const app = express();
app.use(express.json()); // Parse JSON bodies

// Root route to fetch and return Google Calendar events
app.get('/', async (req, res) => {
  console.log('Request received at /');
  try {
    const response = await calendar.events.list({
      calendarId: 'primary',
      timeMin: new Date().toISOString(), // List events starting from now
      maxResults: 2, // Limit to 10 events
      singleEvents: true, // Expand recurring events into instances
      orderBy: 'startTime', // Order by start time
    });
    console.log('Events fetched successfully:', response.data.items);
    res.json(response.data.items); // Send events back as JSON
  } catch (error) {
    console.error('Error fetching events:', error.message);
    res.status(500).send('Error fetching events');
  }
});

// Health check endpoint
app.get('/health', (req, res) => {
  console.log('Health endpoint hit');
  res.send('Server is running!');
});

// Start the server
const PORT = 3000;
app.listen(PORT, (err) => {
  if (err) {
    console.error('Error starting server:', err);
  } else {
    console.log(`Server is running on http://localhost:${PORT}`);
  }
});


// Route to add an event
// app.post('/add-event', async (req, res) => {
//   const { summary, start, end } = req.body; // Receive event details from TinyZero
//   try {
//     const event = {
//       summary,
//       start: { dateTime: start, timeZone: 'UTC' },
//       end: { dateTime: end, timeZone: 'UTC' },
//     };
//     const response = await calendar.events.insert({
//       calendarId: 'primary',
//       resource: event,
//     });
//     res.json({ message: 'Event created', event: response.data });
//   } catch (error) {
//     console.error('Error adding event:', error);
//     res.status(500).send('Error adding event');
//   }
// });

// ==================== BELOW IS TO TEST IF  CAN CONNECT WITH GOOGLE API AND GET A RESPONSE BACK ==============

// const { google } = require('googleapis');

// const oauth2Client = new google.auth.OAuth2(
//   '116505421886-b9tbkpm7f5b2esobbcsjuo0h6ghistn3.apps.googleusercontent.com',
//   'change to your own',
//   'http://localhost:3000/oauth2callback'
// );
// oauth2Client.setCredentials({
//   refresh_token: '1//0gplRQpmlQv8OCgYIARAAGBASNwF-L9Ir9-BZj2T-Sqpzc1aTjuKLmITf9pcbY1dugJFBtUkJTrHZDuXDywF_PLy1iN9OSqx_-GQ',
// });

// const calendar = google.calendar({ version: 'v3', auth: oauth2Client });

// async function testCalendar() {
//   try {
//     const response = await calendar.events.list({
//       calendarId: 'primary',
//       timeMin: new Date().toISOString(),
//       maxResults: 10,
//       singleEvents: true,
//       orderBy: 'startTime',
//     });
//     console.log('Events:', response.data.items);
//   } catch (err) {
//     console.error('Error fetching events:', err.message);
//     console.error('Full error:', err);
//   }
// }

// testCalendar();
