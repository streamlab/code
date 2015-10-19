var riffle2 = [];
var riffle4 = [];
var riffle6 = [];

$.getJSON('data/data.json',function(readings) {

  for (i = 0; i <= readings.length - 1; i++) {
    
    // parse the timestamp into moment.js
    var readingtime = moment.utc(readings[i].timestamp);
    
    if (readingtime.isAfter('2015-10-05 19:01:00') && readingtime.isBefore('2015-10-14')) {
    
      // convert timestamp to Unix time (milleseconds since Jan 1, 1970)
      readingtime = readingtime.valueOf();
      
      // account for shift in time zone to make local time EDT
      readingtime = readingtime - (1000*60*240);
  
      
      if (readings[i].device_id == "RiffleBottle2") {
        riffle2.push([readingtime, parseFloat(readings[i].battery)]);
      }
    
      if (readings[i].device_id == "RiffleBottle4") {
        riffle4.push([readingtime, parseFloat(readings[i].battery)]);
      }
    
      if (readings[i].device_id == "RiffleBottle6") {
        riffle6.push([readingtime, parseFloat(readings[i].battery)]);
      }
    }
  
  }
  
  $(function () {
      $('#container').highcharts({
          chart: {
            type: 'spline',
            height: 100
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
          yAxis: {
              title: {
                  text: 'Conductivity (Hz)'
              },
              min: 0,
              max: 600
          },
          tooltip: {
              headerFormat: '<b>{series.name}</b><br>',
              pointFormat: '{point.x:%b %e, %l:%M %P EDT}<br>Reading: {point.y:.2f} %'
          },

          plotOptions: {
              spline: {
                  marker: {
                    enabled: true,
                    symbol: 'circle',
                    radius: 2
                  },
                  lineWidth: 1
              }
          },

          series: [{
            name: "Above Ind Site & Dam",
            data: riffle6,
            color: '#7fc97f'

          }, {
            name: "Between Ind Site & Dam",
            data: riffle4,
            color: '#beaed4'
          },{ 
            name: "Below Dam",
            data: riffle2,
            color: '#fdc086'
          }]
      });
  });

});

