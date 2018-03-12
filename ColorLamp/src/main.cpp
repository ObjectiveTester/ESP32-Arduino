//
// RGB LED controller via web interface
// Steve Mellor 12 Mar 2018
//
// based on Arduino.cc WebServer example
// created 18 Dec 2009
// by David A. Mellis
// modified 9 Apr 2012
// by Tom Igoe
// modified 02 Sept 2015
// by Arturo Guadalupi
//
//
//
//
//
#include <Arduino.h>
#include "WiFi.h"

//network config
const char* ssid     = "IOT";
const char* password = "allthethings";
WiFiServer server(80);

//hardware config
#define RED_PIN 13
#define GREEN_PIN 12
#define BLUE_PIN 14
int redChannel = 0;
int greenChannel = 1;
int blueChannel = 2;

//initial config
boolean cycle = false;
int redValue = 128;
int greenValue = 128;
int blueValue = 128;

String dec2hex(int val)
{
  String hex = String(val, HEX);
  if (hex.length() < 2)
    hex = "0" + hex;
  return hex;
}
String rgb = dec2hex(redValue) + dec2hex(greenValue) + dec2hex(blueValue);

void setColor()
{
  //ifdef for common cathode

  ledcWrite(redChannel, redValue);
  ledcWrite(greenChannel, greenValue);
  ledcWrite(blueChannel, blueValue);
}

void setup()
{
  Serial.begin(115200);
  pinMode(2, OUTPUT);

  ledcSetup(redChannel, 5000, 8);
  ledcSetup(greenChannel, 5000, 8);
  ledcSetup(blueChannel, 5000, 8);
  ledcAttachPin(RED_PIN, 0);
  ledcAttachPin(GREEN_PIN, 1);
  ledcAttachPin(BLUE_PIN, 2);

  setColor();

  delay(10);

  //connect to network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  digitalWrite(2,1);
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();
}

void sendResponse(WiFiClient client)
{
  // send the lamp page
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();
  client.println("<html>");
  client.println("<script>");
  client.println("var co, cy, nc, st;");
  client.println("var def = \"#" + rgb + "\";");
  client.println("var req = new XMLHttpRequest();");
  client.println();
  client.println("window.addEventListener(\"load\", startup, false);");
  client.println("function startup() {");
  client.println("  co = document.querySelector(\"#color\");");
  client.println("  co.value = def;");
  client.println("  co.addEventListener(\"change\", chColor, false);");
  client.println("  co.select();");
  client.println("  cy = document.querySelector(\"#cycle\");");
  client.println("  cy.addEventListener(\"change\", chCycle, false);");
  client.println("  cy.select();");
  client.println("}");
  client.println("function chColor(event) {");
  client.println("  console.log(co.value);");
  client.println("  nc = co.value.replace(\"#\", \"\");");
  client.println("  document.getElementById(\"cycle\").checked = false;");
  client.println("  req.open('GET', '/?color='+nc, true);");
  client.println("  req.send();");
  client.println("}");
  client.println("function chCycle(event) {");
  client.println("  console.log(cy.checked);");
  client.println("  st = cy.checked;");
  client.println("  setTimeout(function(){location.reload(true)}, 250);"); //reloads client side to show current color
  client.println("  req.open('GET', '/?cycle='+st, true);");
  client.println("  req.send();");
  client.println("}");
  client.println("</script>");
  client.println("<input type=\"color\" id=\"color\">");
  client.print("<input type=\"checkbox\" id=\"cycle\"");
  if (cycle)
    client.print(" checked");
  client.println(">");
  client.println("</html>");
}

void updateColor(String buffer)
{
  if (buffer.startsWith("color"))
  {
    rgb = buffer.substring(1 + buffer.indexOf("="), 7 + buffer.indexOf("="));
    Serial.println(rgb);
    redValue = strtol(rgb.substring(0, 2).c_str(), 0, 16);
    greenValue = strtol(rgb.substring(2, 4).c_str(), 0, 16);
    blueValue = strtol(rgb.substring(4, 6).c_str(), 0, 16);

    setColor();
  }
}

void loop()
{
  // listen for incoming clients
  WiFiClient client = server.available();

  if (client)
  {
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    String req_str = "";

    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        req_str += c;

        if (c == '\n' && currentLineIsBlank && req_str.startsWith("GET"))
        {
          sendResponse(client);
          if (req_str.indexOf("color") > 0)
          {
            String req = String(req_str.substring(req_str.indexOf("color"), req_str.indexOf(" HTTP")));
            updateColor(req);
          }
          if (req_str.indexOf("cycle") > 0)
          {
            if (req_str.substring(req_str.indexOf("cycle"), req_str.indexOf(" HTTP")).endsWith("true"))
            {
              cycle = true;
              if ((redValue > 0) && (blueValue > 0) && (greenValue > 0))
              {

                if ((blueValue < redValue) && (blueValue < greenValue))
                {
                  blueValue = 0;
                }
                else if ((greenValue < redValue) && (greenValue < blueValue))
                {
                  greenValue = 0;
                }
                else
                  redValue = 0;
              }
            }
            else
            {
              cycle = false;
              rgb = dec2hex(redValue) + dec2hex(greenValue) + dec2hex(blueValue);
            }
          }
          break;
        }
        if (c == '\n')
        {
          // you're starting a new line
          currentLineIsBlank = true;
        }
        else if (c != '\r')
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }

    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
  if (cycle)
  {
    //red - orange - yellow
    if ((redValue > greenValue) && (blueValue == 0))
    {
      greenValue++;
    }
    else if ((redValue <= greenValue) && (blueValue == 0))
    {
      redValue--;
    }

    //yellow - green - blue
    if ((blueValue < greenValue) && (redValue == 0))
    {
      blueValue++;
    }
    else if ((blueValue >= greenValue) && (redValue == 0))
    {
      greenValue--;
    }

    //blue - violet - red
    if ((blueValue > redValue) && (greenValue == 0))
    {
      redValue++;
    }
    else if ((blueValue <= redValue) && (greenValue == 0))
    {
      blueValue--;
    }

    setColor();
    delay(5);
  }
}
