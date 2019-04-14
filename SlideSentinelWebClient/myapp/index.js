var map;
var markers = [];
var JSONData;
var customSheetName = 'data';
var redIcon = "redIcon.png";
var blueIcon = "blueIcon.png";
var yellowIcon= "yellowIcon.png";
var greenIcon = "greenIcon.png";
var icons = {
          red: {
            name: 'Differential',
            icon: 'redIcon.png'
          },
          blue: {
            name: 'RTK',
            icon: 'blueIcon.png'
          },
          green: {
            name: 'RTK Differential',
            icon: 'greenIcon.png'
          }
        };

function loadData() {
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if (this.readyState == 4 && this.status == 200) {
			JSONData = JSON.parse(this.responseText);
			clearMarkers();
			setMarkers(JSONData);
		}
	};
	console.log(customSheetName);
	xhttp.open("Get", 'main.js?'+customSheetName,true);
	xhttp.send();
}
function loadSheetNames() {
	var xhttp = new XMLHttpRequest();
	xhttp.onreadystatechange = function() {
		if(this.readyState ==4) {

			setSheetNames(this.responseText);
		}
	};
	xhttp.open("Get", 'loadSheetNames',true);
	xhttp.send();
}
function setSheetNames(sheetNames)
{
	sheetNames = sheetNames.split(",");
	var newLink;
	var evt;
	for (var i=0;i<sheetNames.length;i++)
	{
		tmpName = sheetNames[i];
		newLink = document.createElement("a");
		newLink.setAttribute('href',"#");
		newLink.setAttribute('display',"none");
		newLink.innerHTML = sheetNames[i];
		newLink.onclick = function (evt) {
			customSheetName = evt.srcElement.innerHTML
			document.getElementById("mainDropBtn").innerHTML = "Sheet:"+customSheetName;
			
		};
		document.getElementById("dropdown-content").appendChild(newLink);
	}
	
	
	
}
function dropbtnOnClick()
{
	document.getElementById("dropdown-content").classList.toggle("show");
}
window.onclick = function(e) {
  if (!e.target.matches('.dropbtn')) {
  var myDropdown = document.getElementById("dropdown-content");
    if (myDropdown.classList.contains('show')) {
      myDropdown.classList.remove('show');
    }
  }
}

function findColumnName(columnNames, target)
{
	for (var i=0; i<columnNames.length; i++) {
		if (columnNames[i].toLowerCase() == target.toLowerCase())
		{
			return i;
		}
	}
}
function initMap() {
	var mapOptions = {
		center: {lat: 44, lng: -122},
		zoom: 8
	}
	map = new google.maps.Map(document.getElementById('map'), mapOptions );
	var legend = document.getElementById('legend');
	for (var i in icons) {
		var type = icons[i];
          var name = type.name;
          var icon = type.icon;
          var div = document.createElement('div');
          div.innerHTML = '<img src="' + icon + '"> ' + name;
          legend.appendChild(div);
	}
	map.controls[google.maps.ControlPosition.RIGHT_BOTTOM].push(legend);
}
function setMarkers(data) {

	var i = 0;
	var j = 0;
	var count = 0;
	var customSymbol = {
    path: google.maps.SymbolPath.BACKWARD_CLOSED_ARROW,
    fillColor: 'rgb(65, 193, 244)',
    scale: 5,
    strokeColor: 'rgb(65, 193, 244)',
    strokeWeight: 1
  };

	columnNames = data[0];
	latPos = findColumnName(columnNames,"latitude");
	longPos = findColumnName(columnNames,"longitude");
	NodePos = findColumnName(columnNames,"Node ID");
	modePos = findColumnName(columnNames,"Mode");
	data.shift(); //removing first row, which is the headers
	console.log(data[0]);
    for (var i in data) {
		var newLocation = data[i];
		var newLatLng = new google.maps.LatLng((newLocation[latPos]),(newLocation[longPos]));
		var isNew = 1;
		//console.log('newLocation:'+newLatLng);
		for (var j in markers)
		{
			if((markers.length != 0)&&(typeof(newLatLng.lat()) == "number")&&(typeof(newLatLng.lng()) == "number"))
			//console.log('checking against:'+markers[j].position);
			if(newLatLng.equals(markers[j].position))
			{
				console.log('not new');
				isNew = 0;
			}
			else
			{
				
			}
		}
        
		if(isNew)
		{
			++count;
			var marker = new google.maps.Marker({
				position: newLatLng,
				map: map,
				animation: google.maps.Animation.DROP,
				title: 'title',
				label: 'node: '+newLocation[NodePos],
				zIndex: 1,
				icon: customSymbol
			});
			
			if(newLocation[modePos] =='RTK')
			{
				marker.setIcon(blueIcon);
			}
			else if(newLocation[modePos] =='Float RTK')
			{
				marker.setIcon(greenIcon);
			}
			marker.addListener('click', function() {
				if(document.getElementById("displayTable"))
				{
					var myNode = document.getElementById("selectorDisplayField");
					while (myNode.firstChild) {
						myNode.removeChild(myNode.firstChild);
					}
				}
				var table = document.createElement("TABLE");
				table.setAttribute("id", "displayTable");
				table.style.border = "1px solid #000"
				document.getElementById("selectorDisplayField").appendChild(table);
				var newRow = document.createElement("TR");
				newRow.style.border = "1px solid #000"
				newRow.setAttribute("id","headers");
				table.appendChild(newRow);
				var i = 0;
				for (var i in columnNames)
				{
					newColumn = document.createElement("TD");
					newColumn.style.border = "1px solid #000";
					newRow.appendChild(newColumn);
					newText = document.createTextNode(columnNames[i]);
					newColumn.appendChild(newText);
					
				}
				var i = 0;
				var newRow = document.createElement("TR");
				table.appendChild(newRow); 
				for (var i in this.newLocation)
				{
					newColumn = document.createElement("TD");
					newColumn.style.border = "1px solid #000";
					newRow.appendChild(newColumn);
					newText = document.createTextNode(this.newLocation[i]);
					newColumn.appendChild(newText);
				}
				
			}.bind({newLocation:newLocation}));
			// Push marker to markers array
			markers.push(marker);
		}
    }
}
function clearMarkers() {
 
    // Loop through markers and set map to null for each
    for (var i=0; i<markers.length; i++) {
     
        markers[i].setMap(null);
    }
    
    // Reset the markers array
    markers = [];
}
