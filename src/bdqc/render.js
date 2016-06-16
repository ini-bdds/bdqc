
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
  * Re: the rendering (and D3) implementation:
  * 1. the received data is column-oriented in the sense that summary.stats
  *    is a list of objects which will correspond to columns, and, in
  *    particular, each column contains references (by index) to the paths
  *    that are anomalies.
  *    The incidence must be constructured from this.
  * 2. In all cases, vectors of cells (svg:rects) are parented by g elements
  *    so moving such vectors in the display is trivial: merely change the
  *    enclosing svg:g's transform; cells' transforms need not be touched.
  *    However, this simplicity is only available to one dimension. To
  *    reorder the other dimension, ALL cells' transforms must be changed.
  */

const CELLSIZE = 16;

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
var svg_matrix = null;
var svg_dplots = d3.select("svg#dplots");

function render( summary ) { // alternatively, d3.json( data_file, function( summary ) {} )

	var stats = summary.stats;
	var paths = Array(summary.paths.length) // summary.paths;

	const NR = paths.length;
	const NC = stats.length;

	gridW = CELLSIZE*NC;
	gridH = CELLSIZE*NR;

	svg_matrix = d3.select("svg#matrix").attr("width",  gridW + margin.left + margin.right)
		.attr("height", gridH + margin.top + margin.bottom)
		.style("margin-left", -margin.left + "px")
		.append("g")
		.attr("transform", "translate(" + margin.left + "," + margin.top + ")");

	hpos = d3.scale.ordinal().rangeBands([0, gridW]);
	vpos = d3.scale.ordinal().rangeBands([0, gridH]);

	/* Set header text according to which of the three BDQC summary types we're showing. */
	d3.select("h1#datatype").text( summary.type );

	/* Modify the pre-existing svg:def to fit the computed cell size. */
	d3.select("#crossout").attr("p","M 0 0 L "+hpos.rangeBand()+" "+hpos.rangeBand() );

	/* Make a local version of the summary.paths that includes anomaly counts. */
	summary.paths.forEach( function( summary_name, i ) {
		paths[i] = { name:summary_name, anomcount:0 } } );
	/* ...and tally the anomalies per path. */
	stats.forEach( function( obj, i ) {
		obj.anoms.forEach( function(pi) { paths[pi].anomcount += 1; } ) } );

	/* Precompute the possible element-to-position maps. */
	var corders = {
		plugin: d3.range(NC).sort(function(a, b) { 
			return d3.ascending(stats[a].plugin+"/"+stats[a].name, stats[b].plugin+"/"+stats[b].name); }),
		nanoms: d3.range(NC).sort(function(a, b) {
			return stats[b].anoms.length - stats[a].anoms.length; })
	};

	var rorders = {
		path: d3.range(NR).sort(function(a, b) {
			return d3.ascending(paths[a].name, paths[b].name); }),
		nanoms: d3.range(NR).sort(function(a, b) {
			return paths[b].anomcount - paths[a].anomcount; })
	}

	/* ...and install the default sort order. These are subject to
	   change when we reorder at end of initialization, but we need
	   to install *some* order so that subsequent rangeBand() calls
	   return finite values! */
	hpos.domain( corders.nanoms );
	vpos.domain( rorders.nanoms );

	/**
	  * Create the grid background.
	  */

	svg_matrix.append("rect")
		.attr("class", "background")
		.attr("width",  gridW)
		.attr("height", gridH);

	/**
	  * Create the columns...
	  */

	var allCols = svg_matrix.selectAll(".column")
		.data(stats)
		.enter()
		.append("g")
		.attr("class", "column")
		.each( createCol )
		.on("click", clickCol );

	function clickCol( col ) {
		// this: svg:g.row, row: [Object] ...a row of the matrix
	}

	/**
	  * ...and populate each column with cells corresponding to anomalies.
	  */

	function createCol( stat, i ) {
		d3.select(this).selectAll(".cell")
			.data( stat.anoms )
			.enter()
			.append("rect")
			.attr("class", "cell")
			.attr("width",  hpos.rangeBand())
			.attr("height", vpos.rangeBand())
			.style("fill", "slateblue" )
			.on("click",     clickCell )
			.on("mouseover", mouseOverCell )
			.on("mouseout",  mouseExitCell );
	}

	// The cell arg in the following is the __data__ cached in the rect
	// which is an integer index into the paths array.
	// FYI, 'this' is the svg:rect.cell itself, and the svg:rects are
	// children of svg:g.column elems.

	function clickCell( cell ) {

		var g = this.parentNode
		var col = g.__data__

		// Make visible the corresponding density plot (svg:g element).
		svg_dplots.selectAll("g.dplot")
			.attr("visibility", function(d,i) { return d == col ? "visible" : "hidden" } );

		// Update the density plot heading.
		d3.select("h2#dplot").text( col.plugin+"/"+col.name );

		// Update the column highlighting.
		d3.selectAll(".column text").classed( "active", false );
		d3.select( g ).selectAll( "text" ).classed( "active", true );
	}

	function mouseOverCell( cell ) {
		d3.selectAll(".row text").classed( "hover", function( d, i ) { return i == cell; });
		d3.select( this.parentNode ).selectAll( "text" ).classed( "hover", true );
	}

	function mouseExitCell() {
		d3.select( this.parentNode ).selectAll( "text" ).classed( "hover", false );
	}

	allCols.append("line").attr("y2", gridH);
	allCols.append("text")
		.attr("x", +6)
		.attr("y", hpos.rangeBand() / 2)
		.attr("dy", ".32em")
		.attr("text-anchor", "start")
		.attr("transform", function(d, i) { return "rotate(-90)"; })
		.text( function( d, i) { return d.plugin + "/" + d.name; });

	/**
	  * Rows are really only created for the column headers. Unlike columns,
	  * they don't contain child elements in the grid frame; they only
	  * contain the divider lines and margin text appended below.
	  */

	var allRows = svg_matrix.selectAll(".row")
		.data(d3.range(NR)) // Note: matrix can be replaced by d3.range(n), unless
		.enter()      // construction code and/or event handlers need access
		.append("g")  // to the data.
		.attr("class", "row")
		.attr("transform", function(d, i) { return "translate(0," + vpos(i) + ")"; })
		.on( "click", clickRow );

	function clickRow( row ) {
		// this: svg:g.column, col:int (because that's all we add above)
	}

	allRows.append("line").attr("x1", gridW);
	allRows.append("text")
		.attr("x", -6)
		.attr("y", vpos.rangeBand() / 2)
		.attr("dy", ".32em")
		.attr("text-anchor", "end")
		.text(function(d, i) { return paths[i].name; });

	d3.select("#rowopt").on("change", function() {
		//clearTimeout(timeout);
		reorder_rows(this.value, 1000 );
	});

	d3.select("#colopt").on("change", function() {
		//clearTimeout(timeout);
		reorder_cols(this.value, 1000 );
	});

	function reorder_rows( orderselector, ms ) {

		vpos.domain(rorders[ orderselector ]);

		var t = svg_matrix.transition().duration( ms );

		t.selectAll(".cell")
			.attr( "y", function( d, i ) { 
					return vpos(d); });
		t.selectAll(".row")
			.attr("transform", function(d, i) {
					return "translate(0," + vpos(d) + ")"; });
	}
	
	function reorder_cols( orderselector, ms ) {

		hpos.domain(corders[ orderselector ]);

		var t = svg_matrix.transition().duration( ms );

		t.selectAll(".column")
			.attr("transform", function( d, i ) { 
					return "translate("+hpos(i)+",0)"; })

	}

	/***********************************************************************
	  * Pre-render the density plots. UI merely makes (in)visible the svg:g
	  * elements containing different plots.
	  */

	svg_dplots = d3.select("svg#dplots")

	var plots = svg_dplots.selectAll(".dplot")
		.data(stats)
		.enter()
		.append("g")
		.attr("visibility", "hidden" )
		.attr("class", "dplot" )
		.attr("transform", "translate(0,160)")// TODO: remove 160 hardcoding
		.each( renderLine );

	function renderLine( d, i ) {
		// d: current datum, i: index, this:DOM element
		x = d.density.map(function(pair){return pair[0]})
		y = d.density.map(function(pair){return pair[1]})

		if( d.density.length > 0 ) {

			w = 272; // width of div (entirely filled by svg element)
			h = 160; // height of div (entirely filled by svg element)
			m = 8;   // margin on all sides of plotting subarea of svg
			sx = d3.scale.linear().domain([d3.min(x), d3.max(x)]).range([0+m, w-m]);
			sy = d3.scale.linear().domain([d3.min(y), d3.max(y)]).range([0+m, h-m]);
			var line = d3.svg.line()
				.x(function(d) { return      sx(d[0]); })
				.y(function(d) { return -1 * sy(d[1]); });
			d3.select(this).append("svg:path")
				.attr("d", line(d.density))
				.attr("stroke","black")
				.attr("fill","none")
				.attr("stroke-width","1");

		} else {

			d3.select(this)
				.append("text").text("non-quantitative")
				.attr("y", -10 )
		}
	}

	reorder_rows( "nanoms", 0 );
	reorder_cols( "nanoms", 0 );

	//console.log( "executed at " + Date().toString() );
}

render( DATA );

