
var visible_plot = null;

function handleTableCellEnter( e ) {

	// Hide the currently-visible...

	if( visible_plot != null ) {
		visible_plot.style.visibility = 'hidden';
		visible_plot = null;
	}

	var pair = e.target.id.split('_');
	var rugindex  = pair[0].substring(1); // lop off the "c" prefix
	var plotindex = pair[1];

	// ...and record and display the newly-selected one.
	visible_plot = document.getElementById( 'g'+plotindex )
	visible_plot.style.visibility = 'visible';

	// The xOutlier classes are numbered, so to get them all but NOT
	// axis lines...
	d3.selectAll("line[class^=xOutlier]").attr("visibility","hidden");
	d3.select(visible_plot).selectAll(".xOutlier"+rugindex).attr("visibility","visible");
}


/**
  *
  */
function initDocument() {

	// Insure all plots are initially hidden.

	var plots_div = document.getElementById("plots");
	var nodes = plots_div.getElementsByTagName("svg");
	for(var i = 0; i < nodes.length; i++ ) {
		nodes.item(i).style.visibility = 'hidden';
	}

	// Add mouseenter handler to all incidence matrix data cells.

	var table = document.getElementById("incidence_matrix");
	var table_cells = table.getElementsByTagName("td");
	for(var i = 0; i < table_cells.length; i++ ) {
		if( table_cells.item(i).id ) {
			table_cells.item(i).onmouseenter = handleTableCellEnter;
		}
	}

	// Render the density plot of each quantitative statistic.

	for(var i = 0; i < density.length; i++ ) {

		var g = d3.select( "g#g"+i )
		// Split the array of points into parallel arrays of coords.
		x = density[i].map(function(pair){return pair[0]})
		y = density[i].map(function(pair){return pair[1]})

		w = 320;
		h = 240;
		m = 20; // margin
		sx = d3.scale.linear().domain([d3.min(x), d3.max(x)]).range([0+m, w-m]);
		sy = d3.scale.linear().domain([d3.min(y), d3.max(y)]).range([0+m, h-m]);
		var line = d3.svg.line()
			.x(function(d) { return      sx(d[0]); })
			.y(function(d) { return -1 * sy(d[1]); });
		g.append("svg:path").attr("d", line(density[i]));
		g.selectAll(".xOutlier")
			.data(outlier[i])
			.enter().append("svg:line")
			.attr("class", function(d,i) { return "xOutlier"+i } )
			.attr("x1", function(d) { return sx(d); })
			.attr("y1", -1 * sy(   0))
			.attr("x2", function(d) { return sx(d); })
			.attr("y2", "-10" );

		// X-axis 
		g.append("svg:line")
			.attr("x1",      sx(0))
			.attr("y1", -1 * sy(0))
			.attr("x2",      sx(d3.max(x)))
			.attr("y2", -1 * sy(0));
		 
		// Y-axis 
		g.append("svg:line")
			.attr("x1",      sx(0))
			.attr("y1", -1 * sy(0))
			.attr("x2",      sx(0))
			.attr("y2", -1 * sy(d3.max(y)));
/*
		g.selectAll(".xLabel")
			.data(sx.ticks(5))
			.enter().append("svg:text")
			.attr("class", "xLabel")
			.text(String)
			.attr("x", function(d) { return sx(d) })
			.attr("y", 0)
			.attr("text-anchor", "middle");
 
		g.selectAll(".yLabel")
			.data(sy.ticks(4))
			.enter().append("svg:text")
			.attr("class", "yLabel")
			.text(String)
			.attr("x", 0)
			.attr("y", function(d) { return -1 * sy(d) })
			.attr("text-anchor", "right")
			.attr("dy", 4);
*/
		g.selectAll(".xTicks")
			.data(sx.ticks(5))
			.enter().append("svg:line")
			.attr("class", "xTicks")
			.attr("x1", function(d) { return sx(d); })
			.attr("y1", -1 * sy(0))
			.attr("x2", function(d) { return sx(d); })
			.attr("y2", "-17");
		 
		g.selectAll(".yTicks")
			.data(sy.ticks(4))
			.enter().append("svg:line")
			.attr("class", "yTicks")
			.attr("y1", function(d) { return -1 * sy(d); })
			.attr("x1", sx(-0.3))
			.attr("y2", function(d) { return -1 * sy(d); })
			.attr("x2", sx(0));
	}
}

