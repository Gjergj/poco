//
// ComboBoxCellRenderer.cpp
//
// $Id: //poco/Main/WebWidgets/ExtJS/src/ComboBoxCellRenderer.cpp#3 $
//
// Library: ExtJS
// Package: Core
// Module:  ComboBoxCellRenderer
//
// Copyright (c) 2007, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
// 
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//


#include "Poco/WebWidgets/ExtJS/ComboBoxCellRenderer.h"
#include "Poco/WebWidgets/ExtJS/TextFieldCellRenderer.h"
#include "Poco/WebWidgets/ExtJS/FormRenderer.h"
#include "Poco/WebWidgets/ExtJS/Utility.h"
#include "Poco/WebWidgets/ComboBoxCell.h"
#include "Poco/WebWidgets/ComboBox.h"
#include "Poco/WebWidgets/WebApplication.h"
#include "Poco/WebWidgets/RequestHandler.h"
#include "Poco/NumberFormatter.h"
#include "Poco/Delegate.h"
#include <map>


namespace Poco {
namespace WebWidgets {
namespace ExtJS {


const std::string ComboBoxCellRenderer::EV_SELECTED("select");
const std::string ComboBoxCellRenderer::EV_BEFORESELECT("beforeselect");
const std::string ComboBoxCellRenderer::EV_AFTERLOAD("load");


ComboBoxCellRenderer::ComboBoxCellRenderer()
{
}


ComboBoxCellRenderer::~ComboBoxCellRenderer()
{
}


JSDelegate ComboBoxCellRenderer::createSelectedServerCallback(const ComboBox* pCombo)
{
	//select : ( Ext.form.ComboBox combo, Ext.data.Record record, Number index )
	static const std::string signature("function(combo,rec,idx)");
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(ComboBoxCell::FIELD_VAL, "+escape(rec.get('d'))"));
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, ComboBoxCell::EV_SELECTED));
	return Utility::createServerCallback(signature, addParams, pCombo->id(), pCombo->selected.getOnSuccess(), pCombo->selected.getOnFailure());
}


Poco::WebWidgets::JSDelegate ComboBoxCellRenderer::createBeforeSelectServerCallback(const ComboBox* pCombo)
{
	// beforeselect : ( Ext.form.ComboBox combo, Ext.data.Record record, Number index )
	// return false to forbid it
	static const std::string signature("function(combo,rec,idx)");
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(ComboBoxCell::FIELD_VAL, "+escape(rec.get('d'))"));
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, ComboBoxCell::EV_BEFORESELECT));
	return Utility::createServerCallback(signature, addParams, pCombo->id(), pCombo->beforeSelect.getOnSuccess(), pCombo->beforeSelect.getOnFailure());
}


Poco::WebWidgets::JSDelegate ComboBoxCellRenderer::createAfterLoadServerCallback(const ComboBox* pCombo)
{
	poco_check_ptr (pCombo);
	static const std::string signature("function(aStore, recs, op)");
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID, ComboBoxCell::EV_AFTERLOAD));
	return Utility::createServerCallback(signature, addParams, pCombo->id(), pCombo->afterLoad.getOnSuccess(), pCombo->afterLoad.getOnFailure());
}


void ComboBoxCellRenderer::renderHead(const Renderable* pRenderable, const RenderContext& context, std::ostream& ostr)
{
	poco_assert_dbg (pRenderable != 0);
	poco_assert_dbg (pRenderable->type() == typeid(Poco::WebWidgets::ComboBoxCell));
	ComboBoxCell* pCell = const_cast<ComboBoxCell*>(static_cast<const Poco::WebWidgets::ComboBoxCell*>(pRenderable));
	//two scenarios: owner is either a ComboBox or a TableColumn!
	View* pOwner = pCell->getOwner();
	ComboBox* pComboOwner = dynamic_cast<ComboBox*>(pOwner);
	poco_check_ptr (pOwner);
	ostr << "new Ext.form.ComboBox({";

	TextFieldCellRenderer::writeCellProperties(pCell, ostr, true, false);
	ostr << ",store:new Ext.data.SimpleStore({fields:['d'],"; //autoLoad:true,
	ostr << "proxy:new Ext.data.HttpProxy({url:";
	std::map<std::string, std::string> addParams;
	addParams.insert(std::make_pair(RequestHandler::KEY_EVID,ComboBoxCell::EV_LOAD));
	
	std::string url(Utility::createURI(addParams, pOwner->id()));
	ostr << url << "}),";
	ostr << "reader:new Ext.data.ArrayReader()";
	if (pComboOwner && pComboOwner->afterLoad.hasJavaScriptCode())
	{
		ostr << ",listeners:{";
		Utility::writeJSEvent(ostr, EV_AFTERLOAD, pComboOwner->afterLoad, &ComboBoxCellRenderer::createAfterLoadServerCallback, pComboOwner);
		ostr << "}";
	}
	
	ostr << "})"; // end SimpleStore
	
	ostr << ",displayField:'d',editable:false"; // editable always false due to ie bug!
	if (!pComboOwner)
		ostr << ",lazyRender:true";
	
	ostr << ",triggerAction:'all'";
	
	std::string tooltip (pCell->getToolTip());
		
	if (pComboOwner && (pComboOwner->selected.hasJavaScriptCode() || !tooltip.empty() || pComboOwner->beforeSelect.hasJavaScriptCode()))
	{
		ostr << ",listeners:{";
		bool written = Utility::writeJSEvent(ostr, EV_SELECTED, pComboOwner->selected, &ComboBoxCellRenderer::createSelectedServerCallback, pComboOwner);
		if (pComboOwner->beforeSelect.hasJavaScriptCode())
		{
			if (written)
				ostr << ",";
			written = Utility::writeJSEvent(ostr, EV_BEFORESELECT, pComboOwner->beforeSelect, &ComboBoxCellRenderer::createBeforeSelectServerCallback, pComboOwner);
		}
		if (!tooltip.empty())
		{
			if (written)
				ostr << ",";
			ostr << "render:function(c){Ext.QuickTips.register({target:c.getEl(),text:'" << Utility::safe(tooltip) << "'});}";
		}
		ostr << "}";
	}

	ostr << "})";
	pCell->beforeLoad += Poco::delegate(&ComboBoxCellRenderer::onLoad);
	WebApplication::instance().registerAjaxProcessor(Poco::NumberFormatter::format(pOwner->id()), pCell);
	if (!pOwner->getName().empty())
	{
		WebApplication::instance().registerFormProcessor(pOwner->getName(), const_cast<ComboBoxCell*>(pCell));
	}
}


void ComboBoxCellRenderer::onLoad(void* pSender, Poco::Net::HTTPServerResponse* &pResponse)
{
	poco_check_ptr (pSender);
	poco_check_ptr (pResponse);
	ComboBoxCell* pCell = reinterpret_cast<ComboBoxCell*>(pSender);
	poco_check_ptr (pCell);
	pResponse->setChunkedTransferEncoding(true);
	pResponse->setContentType("text/javascript");
	std::ostream& out = pResponse->send();
	serialize (pCell, out);
}


void ComboBoxCellRenderer::serialize(const ComboBoxCell* pCell, std::ostream& ostr)
{
	//now serialize data
	std::vector<Any>::const_iterator it = pCell->begin();
	std::vector<Any>::const_iterator itEnd = pCell->end();
	Formatter::Ptr ptrFormatter = pCell->getFormatter();
	ostr << "[";
	for (; it != itEnd; ++it)
	{
		if (it != pCell->begin())
			ostr << ",";
		if (ptrFormatter)
			ostr << "['" << ptrFormatter->format(*it) << "']";
		else
			ostr << "['" << RefAnyCast<std::string>(*it) << "']";
	}
	ostr << "]";
}


void ComboBoxCellRenderer::renderBody(const Renderable* pRenderable, const RenderContext& context, std::ostream& ostr)
{
}


} } } // namespace Poco::WebWidgets::ExtJS
