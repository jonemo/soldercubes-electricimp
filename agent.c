/*
 ElectricImp agent code for imp-based serial communication bridge between
 a Soldercube assembly and the Soldercubes graphical user interface.
 
 Author: Jonas Neubert
 Date: May 2013
 
 */

http.onrequest( function (request,res)
{
  if (request.path == "/write" && request.method == "POST")
  {
    local bytes = null;

    try 
    {  
      // parse input
      try 
      {
        bytes = http.jsondecode(request.body);
      } 
      catch (ex) throw "JSON decoding failed: " + ex;
      
      //validate input
      if (typeof bytes != "array") throw "Incoming data is not an array.";
      
      foreach (i,b in bytes) {
        if (typeof b != "integer") throw "Table entry " + i + " is not an integer.";
        else if (b < 0) throw "Table entry " + i + " is lower than zero.";
        else if (b > 0xFF) throw "Table entry " + i + " is larger than 0xFF.";
      }
      
      // send to device
      device.send("incomingbytes", bytes);
      // log it
      server.log("Agent received write request and sends " + bytes.len() + " to device.");
      // tell server everything went smooth
      res.send(200, "ok");
    }
    catch (ex)
    {
      server.log(ex);
      res.send(200, ex);
      return;
    }
    
    
  }
  else if (request.path == "/read")
  {
    // send back the contents of the outgoingBuffer
    servertable <- server.load();
    // log to server
    server.log("Agent received read request and sends " + servertable.outgoingBuffer.len() + " bytes back.");
    // the reduce function in the next line is equivalent to the javascript join(",") function
    out <- servertable.outgoingBuffer.reduce(function(prevval,curval){return prevval + "," + curval.tostring()})
    res.send(200, out);
    
    // clear outgoingbuffer
    servertable.outgoingBuffer = [];
    server.save(servertable);
  }
  else if (request.path == "/startReceiving")
  {
    server.log("Agent received startReceiving request.");
    servertable <- server.load();
    servertable.receiving <- true;
    // clear outgoingbuffer in case we were receiving because we don't want to flood the client on the first read request
    servertable.outgoingBuffer = [];
    server.save(servertable);
    res.send(200, "ok");
  }
  else if (request.path == "/stopReceiving")
  {
    server.log("Agent received stopReceiving request.");
    servertable <- server.load();
    servertable.receiving <- false;
    server.save(servertable);
    res.send(200, "ok");
  }
});

device.on("outgoingbytes", function (bytes) {
  local out = "";
  
  // if receiving is stopped, we don't do anything
  if (! servertable.receiving) return;
  
  // load our data from storage
  servertable <- server.load();
  
  // if for some reason there is no outgoingBuffer in our data, create it
  if (! servertable.rawin("outgoingBuffer")) {
      servertable.outgoingBuffer <- [];
  }
  
  // append each byte to the outgoing buffer (there might be a more efficient way to do this...
  foreach (i, b in bytes)
  {
    servertable.outgoingBuffer.append(b);
  }
  
  // write the updated data back to storage
  server.save(servertable);
  
  server.log("Agent read " + bytes.len() + " from Imp and now stores " + servertable.outgoingBuffer.len() + " bytes in buffer.");
});

server.log("Agent done starting, URL is: " + http.agenturl());