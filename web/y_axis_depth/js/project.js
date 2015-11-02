var riffle = [];
var riffle2 = [];
var riffle4 = [];
var riffle6 = [];
var second_y = [];


queue()
	.defer(getDepth)
	.defer(getTheJson, "data/data.json")
	.defer(getTheCSV, 1)
	.defer(getTheCSV, 3)
	.await(function(error){
		drawChart();
	});

function getTheJson(filename, callback){
	
	// Live Data JSON processing
	$.getJSON(filename ,function(readings) {

	  for (i = 0; i <= readings.length - 1; i++) {

	    // parse the timestamp into moment.js
	    var readingtime = moment.utc(readings[i].timestamp);

	    if (readingtime.isAfter('2015-10-05 19:01:00') && readingtime.isBefore('2015-10-14')) {

	      // convert timestamp to Unix time (milleseconds since Jan 1, 1970)
	      readingtime = readingtime.valueOf();

	      // account for shift in time zone to make local time EDT
	      readingtime = readingtime - (1000*60*240);


	      if (readings[i].device_id == "RiffleBottle2") {
	        riffle2.push([readingtime, parseFloat(readings[i].conductivity)]);
	      }

	      if (readings[i].device_id == "RiffleBottle4") {
	        riffle4.push([readingtime, parseFloat(readings[i].conductivity)]);
	      }

	      if (readings[i].device_id == "RiffleBottle6") {
	        riffle6.push([readingtime, parseFloat(readings[i].conductivity)]);
	      }
	    }
	  }
	});
	callback(null,null);
}

function getTheCSV(bottle, callback){

	filename = "data/riffle" + bottle + ".csv";
	d3.csv(filename)
		.row(function(readings){
	  return{
	    y: +readings.conductivity * 0.1,
			x: (+readings.utc * 1000) + (1000 * 60 * 240)
	    // x: moment.utc(readings.timestamp).valueOf() - (1000*60*240)
	    }; 
	  })
	.get(function(error, rows){
	  riffle[bottle] = rows;
		callback(null,null);
		});
			
}

function getDepth(callback) {
	filename = "data/depth.csv";
	d3.csv(filename)
		.row(function(readings){
			var timestamp = readings.datetime;
			timestamp = moment(timestamp, "YYYY-MM-DD HH:mm");
			
			return {
				y: +readings.depth,
				x: moment.utc(timestamp).valueOf() - (1000 * 60 * 240)
			};
		})
	.get(function(error, rows){
		second_y = rows;
		callback(null,null);
	});
}

function drawChart(){
  
	$(function () {
	    $('#container').highcharts({
	        chart: {
	          type: 'spline',
	          height: 800
	        },
	        title: {
	            text: 'Live Data: The Monongahela in Morgantown'
	        },
	        subtitle: {
	            text: 'DIY Water Sensors / WVU Streamlab Project / streamlab.cc'
	        },
	        xAxis: {
	            type: 'datetime',
	            dateTimeLabelFormats: { 
	                 day:"%b %e",
	                hour:"%l%P"
	            },
	            title: {
	                text: 'Date/Time'           
	            }
	        },
	        yAxis: [{
	            title: {
	                text: 'Conductivity (Hz)'
	            },
	            min: 0,
	            max: 600
	        },{ // Secondary yAxis
            title: {
                text: 'Depth (feet)',
                style: {
                    color: Highcharts.getOptions().colors[0]
                }
            },
            labels: {
                format: '{value} in',
                style: {
                    color: Highcharts.getOptions().colors[0]
                }
            },
						min: 0,
						max: 10,
            opposite: true
        }],
	        tooltip: {
            headerFormat: '<b>{series.name}</b><br>',
            pointFormat: '{point.x:%b %e, %l:%M %P EDT}<br>Reading: {point.y:.2f}'
	        },

	        plotOptions: {
	            spline: {
	                marker: {
	                  enabled: true,
	                  symbol: 'circle',
	                  radius: 2
	                },
	                lineWidth: 1
	            },
							series: {
								// disable Turbo mode for big sets
								turboThreshold: 0
							}
	        },

	        series: [{
	          name: "Upstream Surface",
	          data: riffle6,
	          color: '#7fc97f'

	        }, {
	          name: "Midstream Surface",
	          data: riffle4,
	          color: '#beaed4'
	        },{
						name: "Midstream Sunk",
						data: riffle[3],
						color: '#9900CC'		
	        },{ 
	          name: "Downstream Surface",
	          data: riffle2,
	          color: '#fdc086'
	        },{
	          name: "Downstream Sunk",
	          data: riffle[1],
						color: '#FF6600'
	        },{
						name: "Depth Above Dam (feet)",
						data: second_y,
						type: 'spline',
						yAxis: 1,
            tooltip: {
                valueSuffix: 'ft'
            }
	        }]
	    });
	});

}

