
/**
  * Three variations of incidence matrix plots (corresponding to the
  * three classes of anomalies) are supported by this code:
  *
  * 1. Incomparable files:
  *		Cells are present (as SVG rects containing "X") for missing data.
  *		Matrix sorted both dimensions by missing data proportion, so
  *		paths and stats with most missing are upper/left.
  *
  * 2. Ambiguous types for statistics:
  *		Cells are present for minority types and are dimmed according
  *		to proportion. Background is saturated.
  *		clicking cell indicates type(?)
  *
  * These first two are somewhat uninteresting and should rarely be needed.
  *
  * 3. Outliers:
  *		Cells are present for outliers.
  *		quantitative - distance from mean indicated by saturation
  *		categorical - same as ambiguous type
  *
  * In all cases, the JSON generator (Python) has minimized the matrix size
  * by only including:
  * 1. rows (files) for which some statistic has anomalies
  * 2. columns (statistics) for which some file contains an anomaly.
  *
  * Colors could be assigned alternatively to directory groups and/or 
  * stats grouped by the providing plugins.
  */

const CELLSIZE = 32;

var margin = {
	top:80,
	right:0,
	bottom:10,
	left:80
};
var gridW = 0;
var gridH = 0;

/**
  * [hv]pos are maps from an index to an actual [a,b] interval in either
  * the horizontal or vertical dimension. The domains of these maps are
  * provided by members of the orders object ("name","count","group").
  * Each member of the orders object contains the indices of cells that
  * have been pre-sorted in one of several ways.
  */
var hpos = null;
var vpos = null;
var svg = null;

//var	opacity = d3.scale.linear().domain([0, 4]).clamp(true);
//var	color = d3.scale.category10().domain(d3.range(10));   // for fill color

// d3.json( data_file, function( summary )
function render( summary ) {

	var matrix = [];
	//const nodes = summary.nodes;
	//var n = nodes.length;
	const NR = summary.paths.length;
	const NC = summary.stats.length;

	var stats = summary.stats;
	var paths = summary.paths;

	gridW = CELLSIZE*NC;
	gridH = CELLSIZE*NR;

	svg = d3.select("svg")
		.attr("width",  gridW + margin.left + margin.right)
		.attr("height", gridH + margin.top + margin.bottom)
		.style("margin-left", -margin.left + "px")
		.append("g")
		.attr("transform", "translate(" + margin.left + "," + margin.top + ")");

	hpos = d3.scale.ordinal().rangeBands([0, gridW]);
	vpos = d3.scale.ordinal().rangeBands([0, gridH]);

	d3.select("#datatype").text("outliers");
	d3.select("#crossout").attr("p","M 0 0 L "+hpos.rangeBand()+" "+hpos.rangeBand() );
	d3.select("h1").text("outliers");

	paths.forEach(function( path, i) {
		// Matrix cells need [c]olumn [i]ndex and [r]ow [i]ndex members.
		matrix[i] = d3.range(NC).map(function(j) { return {si:j, pi:i, value:null}; });
	});

	// Convert links to matrix; count anomalies.
	summary.anoms.forEach(function(a) {
		matrix[a.path][a.stat].value = a.value;
		paths[a.path].nanoms += 1;
		stats[a.stat].nanoms += 1;
	});

	// Precompute the possible orders...
	var corders = {
		plugin: d3.range(NC).sort(function(a, b) { return d3.ascending(stats[a].plugin, stats[b].plugin); }),
		nanoms: d3.range(NC).sort(function(a, b) { return stats[b].nanoms - stats[a].nanoms; }),
	};
	var rorders = {
		dir: d3.range(NR).sort(function(a, b) { return d3.ascending(paths[a].dir, paths[b].dir); }),
		nanoms: d3.range(NR).sort(function(a, b) { return paths[b].nanoms - paths[a].nanoms; }),
	}
	// ...and install the default sort order.
	hpos.domain( corders.plugin );
	vpos.domain( rorders.nanoms );

	/**
	  * Create the grid background.
	  */

	svg.append("rect")
		.attr("class", "background")
		.attr("width",  gridW)
		.attr("height", gridH);

	/**
	  * Create the rows...
	  */

	var allRows = svg.selectAll(".row")
		.data(matrix)
		.enter()
		.append("g")
		.attr("class", "row")
		.attr("transform", function(d, i) { return "translate(0," + vpos(i) + ")"; })
		.each(createRow)
		.on("click", clickRow );
	// Note: allRows is a D3 *group*, so allRows[0].length == row_count.

	/**
	  * ...and the cells as children of rows.
	  */

	function createRow(r) {
		/*var cell = */ d3.select(this).selectAll(".cell")
			// Only matrix elements with non-zero sum-of-link.value become cells.
			.data(r.filter(function(d) { return d.value; }))
			.enter().append("rect")
			.attr("class", "cell")
			.attr("x", function(d) { return hpos(d.si); })
			.attr("width",  hpos.rangeBand())
			.attr("height", vpos.rangeBand())
			//.style("fill-opacity", function(d) { return opacity(d.z); })
			.style("fill", function(d) {
					console.log( "d.value = ", d.value );
					return d.value == null ? null : "red";
					})
			.on("click",     clickCell )
			.on("mouseover", mouseOverCell)
			.on("mouseout",  mouseOutCell );
	}

	function clickRow(p) {
		console.log( p )
	}

	function clickCell(p) {
		console.log( p )
	}

	function mouseOverCell(p) {
		d3.selectAll(".row    text").classed("active", function(d, i) { return i == p.pi; });
		d3.selectAll(".column text").classed("active", function(d, i) { return i == p.si; });
	}

	function mouseOutCell() {
		d3.selectAll("text").classed("active", false);
	}

	allRows.append("line").attr("x2", gridW);
	allRows.append("text")
		.attr("x", -6)
		.attr("y", vpos.rangeBand() / 2)
		.attr("dy", ".32em")
		.attr("text-anchor", "end")
		.text(function(d, i) { p = paths[i];return p.dir+"/"+p.file; });

	/**
	  * Columns are really only created for the column headers.
	  * Unlike rows, they don't contain *graphical* children in
	  * the grid frame.
	  */

	var allCols = svg.selectAll(".column")
		.data(d3.range(NC)) // Note: matrix can be replaced by d3.range(n), unless
		.enter()      // construction code and/or event handlers need access
		.append("g")  // to the data.
		.attr("class", "column")
		.attr("transform", function(d, i) { return "translate(" + hpos(i) + ") rotate(-90)"; })
		.on("click", columnclick );

	function columnclick(e) {
		console.log(e)
	}

	matrix = null; // we don't need it anymore.

	allCols.append("line").attr("x1", -gridH);
	allCols.append("text")
		.attr("x", 6)
		.attr("y", hpos.rangeBand() / 2)
		.attr("dy", ".32em")
		.attr("text-anchor", "start")
		.text(function(d, i) { s = stats[i];return s.plugin+"/"+s.name; });

	d3.select("#rorder").on("change", function() {
		//clearTimeout(timeout);
		reorderrows(this.value);
	});

	function reorderrows( orderselector ) {
		vpos.domain(rorders[ orderselector ]);
		var t = svg.transition().duration(500);
		t.selectAll(".row")
			.attr("transform", function(d, i) { 
					console.log(" row-transit:",d[0],",",i);
					return "translate(0," + vpos(d[0].pi) + ")"; });
	}

	function reordercols( orderselector ) {
		vpos.domain(corders[ orderselector ]);
		var t = svg.transition().duration(500);
		t.selectAll(".row")
			.attr("transform", function(d, i) { 
					console.log(" row-transit:",d[0],",",i);
					return "translate(0," + hpos(d[0].pi) + ")"; })
			.selectAll(".cell")
			.attr("x", function(d, i) { 
					console.log("cell-transit:",d,",",i);
					return hpos(d.si); });
		t.selectAll(".column")
			.attr("transform", function(d, i) { 
					console.log(" col-transit:",d,",",i);
					return "translate(" + hpos(i) + ") rotate(-90)"; });
	}

	//console.log( "executed at " + Date().toString() );
}

render( DATA );


