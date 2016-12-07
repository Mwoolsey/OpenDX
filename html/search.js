<!--

// Define global variables
var SEARCHANY     = 1;
var SEARCHALL     = 2;
var searchType  = '';
var showMatches   = 10;
var currentMatch  = 0;
var copyArray   = new Array();
var docObj      = parent.frames[1].document;

// Determine the type of search, and make
// sure the user has entered something
function validate(entry) {
  if (entry.charAt(0) == "+") {
    entry = entry.substring(1,entry.length);
    searchType = SEARCHALL;
    }
  else { searchType = SEARCHANY; }
  while (entry.charAt(0) == ' ') {
    entry = entry.substring(1,entry.length);
    document.search.query.value = entry;
    }
  while (entry.charAt(entry.length - 1) == ' ') {
    entry = entry.substring(0,entry.length - 1);
    document.search.query.value = entry;
    }
  if (entry.length < 3) {
    alert("Search string too short. Elaborate a little.");
    document.search.query.focus();
    return;
    }
  convertString(entry);
  }

// Put the search terms in an array and
// and call appropriate search algorithm
function convertString(reentry) {
  var searchArray = reentry.split(" ");
  if (searchType == SEARCHALL) { requireAll(searchArray); }
  else { allowAny(searchArray); }
  }

// Define a function to perform a search that requires
// a match of any of the terms the user provided
function allowAny(t) {
  var findings = new Array();
  for (i = 0; i < profiles.length; i++) {
    var compareElement = profiles[i];
    var recordArray = compareElement.split("|");
    for (j = 0; j < t.length; j++) {
      var compareString = t[j].toLowerCase();
      var found = recordArray[2].indexOf(compareString);
      if( found != -1) {
  	  var findObj = new Object();
          findObj.url = recordArray[0];
          findObj.title = recordArray[1];
          var foundSubstr = recordArray[2].substring(found,recordArray[2].length-1);
          var foundSpace = foundSubstr.indexOf(' ');
          findObj.score = parseInt(foundSubstr.substring(foundSpace,foundSubstr.length-1));
          findings[findings.length] = findObj;
      }
    }
  }
  verifyManage(findings);
}

// Define a function to perform a search that requires
// a match of all terms the user provided
function requireAll(t) {
  var findings = new Array();
  for (i = 0; i < profiles.length; i++) {
    var allConfirmation = true;
    var allString       = profiles[i];
    var recordArray = allString.split("|");
    var score = 0;
    for (j = 0; j < t.length; j++) {
      var allElement = t[j].toLowerCase();
      var found = recordArray[2].indexOf(allElement);
      if (found == -1) {
        allConfirmation = false;
        continue;
      } else {
          var foundSubstr = recordArray[2].substring(found,recordArray[2].length-1);
          var foundSpace = foundSubstr.indexOf(' ');
          score += parseInt(foundSubstr.substring(foundSpace,foundSubstr.length-1));
      }
    }
    if (allConfirmation) {
      var findObj = new Object();
          findObj.url = recordArray[0];
          findObj.title = recordArray[1];
          findObj.score = score;
          findings[findings.length] = findObj;
      }
    }
  verifyManage(findings);
}
// Determine whether the search was successful
// If so print the results; if not, indicate that, too
function verifyManage(resultSet) {
  if (resultSet.length == 0) { noMatch(); }
  else {
    copyArray = resultSet.sort(byScore);
    formatResults(copyArray, currentMatch, showMatches);
    }
  }


function byScore(a, b) {
  if (a.score > b.score) return -1;
  if (a.score < b.score) return 1;
  return 0;
}
// Define a function that indicates that the returned no results
function noMatch() {
  //var searchStr = ;
  docObj.open();
  docObj.writeln('<HTML><HEAD><TITLE>Search Results</TITLE>\n' + 
    '<meta http-equiv="content-type" content="text/html;charset=ISO-8859-1">\n</HEAD>' +
    '<BODY BGCOLOR="#FFFFFF" link="#00004b" vlink="#4b004b">' +
    '<TABLE WIDTH=90% BORDER=0 ALIGN=CENTER><TR><TD VALIGN=TOP><B><DL>' +
    '<HR NOSHADE WIDTH=100%>' +
    'returned no results.<HR NOSHADE WIDTH=100%></TD></TR></TABLE></BODY></HTML>');
  docObj.close();
  //document.search.query.select();
  }

// Define a function to print the results of a successful search
function formatResults(results, reference, offset) {
  copyArray = results;
  var currentRecord = (results.length < reference + offset ? results.length : reference + offset);
  docObj.open();
  docObj.writeln('<HTML>\n<HEAD>\n<TITLE>Search Results</TITLE>\n' + 
    '<meta http-equiv="content-type" content="text/html;charset=ISO-8859-1">\n</HEAD>' +
    '<BODY BGCOLOR="#FFFFFF" link="#00004b" vlink="#4b004b">' +
    '<TABLE WIDTH=90% BORDER=0 ALIGN=CENTER CELLPADDING=3><TR><TD>' +
    '<HR NOSHADE WIDTH=100%></TD></TR><TR><TD VALIGN=TOP>' +
    'Search Query: <I>' + '</I><BR>\n' +
    '<B>Search Results: <I>' + (reference + 1) + ' - ' +
    currentRecord + ' of ' + results.length + '</I></B><BR><BR>' +
    '\n\n<!-- Begin result set //-->\n\n\t<DL>');
    for (var i = reference; i < currentRecord; i++) {
       docObj.writeln('\n\n\t<DT>' + 'STIV*: ' + results[i].score + ' -- ');
       docObj.writeln('<I>' + results[i].title + '</I>');
       docObj.writeln('\t<DD><A HREF="' + results[i].url + 
	  	'" TARGET=_top>' + results[i].url + '</A><P>');
      }
  docObj.writeln('\n\t</DL>\n\n<!-- End result set //-->\n\n');
  prevNextResults(results.length, reference, offset);
  docObj.writeln('<HR NOSHADE WIDTH=100%>' +
    '<br><i>*STIV - Search Term Indexing Value</i></p>' +
    '</TD>\n</TR>\n</TABLE>\n</BODY>\n</HTML>');
  docObj.close();
  //document.search.query.select();
  }

// Define a function to dynamically display Prev and Next buttons
function prevNextResults(ceiling, reference, offset) {
  docObj.writeln('<CENTER><FORM>');
  if(reference > 0) {
    docObj.writeln('<INPUT TYPE=BUTTON VALUE="Prev ' + offset + ' Results" ' +
      'onClick="parent.frames[0].formatResults(parent.frames[0].copyArray, ' +
      (reference - offset) + ', ' + offset + ')">');
    }
  if(reference >= 0 && reference + offset < ceiling) {
    var trueTop = ((ceiling - (offset + reference) < offset) ? ceiling - (reference + offset) : offset);
    var howMany = (trueTop > 1 ? "s" : "");
    docObj.writeln('<INPUT TYPE=BUTTON VALUE="Next ' + trueTop + ' Result' + howMany + '" ' +
      'onClick="parent.frames[0].formatResults(parent.frames[0].copyArray, ' +
      (reference + offset) + ', ' + offset + ')">');
    }
  docObj.writeln('</CENTER>');
  }

//-->