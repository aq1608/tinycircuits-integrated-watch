import { google } from 'googleapis';
import open from 'open'; // Import the open library
import readline from 'readline'; // Node.js Core ESM-compatible module

const CLIENT_ID = '116505421886-b9tbkpm7f5b2esobbcsjuo0h6ghistn3.apps.googleusercontent.com';
const CLIENT_SECRET = 'Change to your own';
const REDIRECT_URI = 'http://localhost:3000/oauth2callback';

const oauth2Client = new google.auth.OAuth2(CLIENT_ID, CLIENT_SECRET, REDIRECT_URI);

const SCOPES = ['https://www.googleapis.com/auth/calendar'];

const authUrl = oauth2Client.generateAuthUrl({
  access_type: 'offline', // This is required for a refresh token
  prompt: 'consent', // Force Google to re-show the consent screen
  scope: SCOPES,
});

console.log('Authorize this app by visiting this URL:', authUrl);
open(authUrl); // Open the URL in the default browser

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout,
});

rl.question('Enter the code from that page here: ', async (code) => {
  rl.close();
  try {
    const { tokens } = await oauth2Client.getToken(code);
    console.log('Access Token:', tokens.access_token);
    console.log('Refresh Token:', tokens.refresh_token);
    console.log('Tokens:', tokens);
  } catch (error) {
    console.error('Error retrieving tokens:', error);
  }
});
