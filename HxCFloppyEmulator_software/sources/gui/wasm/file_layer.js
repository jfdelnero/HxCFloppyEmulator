///////////////////////////////////////////////////////////////////////////////////
//-------------------------------------------------------------------------------//
//-------------------------------------------------------------------------------//
//-----------H----H--X----X-----CCCCC----22222----0000-----0000------11----------//
//----------H----H----X-X-----C--------------2---0----0---0----0--1--1-----------//
//---------HHHHHH-----X------C----------22222---0----0---0----0-----1------------//
//--------H----H----X--X----C----------2-------0----0---0----0-----1-------------//
//-------H----H---X-----X---CCCCC-----222222----0000-----0000----1111------------//
//-------------------------------------------------------------------------------//
//----------------------------------------------------- http://hxc2001.free.fr --//
///////////////////////////////////////////////////////////////////////////////////
// File : file_layer.js
// Contains:
//
// Written by: Jean François DEL NERO
//
///////////////////////////////////////////////////////////////////////////////////

function HxCFILE_emscript_js()
{
	this.context = null;

	this.currentModNode = null;
}

HxCFILE_emscript_js.prototype.load = function(input, callback)
{
	var player = this;
	if (input instanceof File)
	{
		// Local file
		var reader = new FileReader();

		reader.onload = function()
		{
			return callback(reader.result);
		}.bind(this);

		reader.readAsArrayBuffer(input);
	}
	else
	{
		// "Http" File.
		var XmlHttpReq = new XMLHttpRequest();

		XmlHttpReq.open('GET', input, true);
		XmlHttpReq.responseType = 'arraybuffer';
		XmlHttpReq.onload = function(e)
		{
			if (XmlHttpReq.status === 200)
			{
				return callback(XmlHttpReq.response);
			}
		}.bind(this);
		XmlHttpReq.send();
	}
}
