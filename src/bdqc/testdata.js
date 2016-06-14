var DATA = {
	"paths":[
		{"dir":".","file":"woz","nanoms":0},
		{"dir":".","file":"foo","nanoms":0},
		{"dir":".","file":"bar","nanoms":0},
		{"dir":".","file":"maz","nanoms":0},
		{"dir":".","file":"jag","nanoms":0},
		{"dir":".","file":"baz","nanoms":0},
		{"dir":".","file":"fip","nanoms":0}
	],
	"stats":[
		{
		"plugin":"p1",
		"name":"abc",
		"density":[],
		"nanoms":0
		},
		{
		"plugin":"p3",
		"name":"xyz",
		"density":[],
		"nanoms":0
		},
		{
		"plugin":"p1",
		"name":"tuv",
		"density":[],
		"nanoms":0
		},
		{
		"plugin":"p2",
		"name":"pqr",
		"density":[],
		"nanoms":0
		},
		{
		"plugin":"p2",
		"name":"ijk",
		"density":[],
		"nanoms":0
		}
	],
	"matrix":[
		[null,null,null,{}  ,null],
		[null,null,null,{}  ,null],
		[null,{}  ,{}  ,null,null],
		[{}  ,null,null,null,null],
		[null,null,{}  ,null,null],
		[null,{}  ,null,null,null],
		[null,{}  ,null,null,{}  ]
	],
	"anoms":[
        {
            "stat": 3,
            "path": 1,
			"value":1
        }, 
        {
            "stat": 4,
            "path": 0,
			"value":1
        }, 
        {
            "stat": 2,
            "path": 5,
			"value":1
        }, 
        {
            "stat": 0,
            "path": 3,
			"value":1
        }, 
        {
            "stat": 1,
            "path": 6,
			"value":1
        }
	],
    "nodes": [
        {
            "group": 1, 
            "name": "Foo"
        }, 
        {
            "group": 2, 
            "name": "Bar"
        }, 
        {
            "group": 1, 
            "name": "Baz"
        }, 
        {
            "group": 2, 
            "name": "Piz"
        }, 
        {
            "group": 1, 
            "name": "Wop"
        }
    ], 
    "links": [
        {
            "source": 1, 
            "target": 0, 
            "value": 1
        }, 
        {
            "source": 2, 
            "target": 0, 
            "value": 8
        }, 
        {
            "source": 3, 
            "target": 2, 
            "value": 10
        }, 
        {
            "source": 4, 
            "target": 2, 
            "value": 6
        }, 
        {
            "source": 4, 
            "target": 3, 
            "value": 1
        }
    ]
}
