/*
* Copyright (C) 2009 Mamadou Diop.
*
* Contact: Mamadou Diop <diopmamadou@yahoo.fr>
*	
* This file is part of Open Source Doubango Framework.
*
* DOUBANGO is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*	
* DOUBANGO is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesser General Public License for more details.
*	
* You should have received a copy of the GNU General Public License
* along with DOUBANGO.
*
*/

/**@file txc_rls.c
 * @brief RFC 4826 subclause 4: <a href="http://tools.ietf.org/html/rfc4826#section-4">RLS Services Documents</a>
 *
 * @author Mamadou Diop <diopmamadou(at)yahoo.fr>
 *
 * @date Created: Sat Nov 8 16:54:58 2009 mdiop
 */

#include "txc_rls.h"
#include "txc_macros.h"
#include "txc.h"

#include "tsk_memory.h"
#include "tsk_string.h"
#include "tsk_macros.h"

#include <string.h>

/**@defgroup txc_rls_group XCAP RLS Services Documents 
*/


/**@page txc_rls_page XCAP RLS Services Documents Tutorial (rls-services)
* @par Application Unique ID (AUID)
* - '<span style="text-decoration:underline;">rls-services</span>' as per rfc 4826 subclause 4.4.1
* @par Default Document Namespace
* - '<span style="text-decoration:underline;">urn:ietf:params:xml:ns:rls-services</span>' as per rfc 4826 subclause 4.4.4
* @par MIME Type
* - '<span style="text-decoration:underline;">application/rls-services+xml</span>' as per rfc 4826 subclause 4.4.2
* @par Default document name
* - '<span style="text-decoration:underline;">index</span>' as per rfc 4826 subclause 4.4.7
*
* <H2>== Deserialize and dump an rls-services document received from an XDMS==</H2>
* @code
#include "txc_api.h" 

txc_rls_t* rls = 0;
tsk_list_item_t* item = 0;
tsk_list_t *list = 0;
printf("\n---\nTEST RLS-SERVICES\n---\n");
{
	// create rls context
	rls = txc_rls_create(buffer, size);

	// get all services
	list = txc_rls_get_all_services(rls);

	// dump services
	tsk_list_foreach(item, list)
	{
		txc_rls_service_t *rls_service = ((txc_rls_service_t*)item->data);
		char* rls_service_str = txc_rls_service_serialize(rls_service);
		printf("\n%s\n", rls_service_str);
		TSK_SAFE_FREE2(rls_service_str);
	}

	// free services
	TSK_LIST_SAFE_FREE(list);
	
	// free rls context
	txc_rls_free(&rls);
}
* @endcode
*/

#define RLS_RETURN_IF_INVALID(rls) if(!rls || !(rls->docPtr)) return 0;

#define RLS_XML_HEADER	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
						"<rls-services xmlns=\""TXC_NS_RLS"\">"
#define RLS_XML_FOOTER	"</rls-services>"

/**@ingroup txc_rls_group
* Internal function to initialize a service . 
* You MUST call @ref TXC_RLS_SERVICE_CREATE to create and initialize your object.
* @param service The service element to initialize
*/
void txc_rls_service_init(txc_rls_service_t *service)
{
	memset(service, 0, sizeof(txc_rls_service_t));
}


/**@ingroup txc_rls_group
* Update an rls service.
* @param service The service to update
* @param uri The new uri.
* @param resource_list The new resource-list
*/
void txc_rls_service_set(txc_rls_service_t *service, const char* uri, const char* resource_list)
{
	if(service)
	{		
		tsk_strupdate2(&(service->uri), uri);
		tsk_strupdate2(&(service->resource_list), resource_list);
	}
}

/**@ingroup txc_rls_group
* Add a new package to a service
* @param service The service to update
* @param package The new package to add to the service
*/
void txc_rls_service_add_package(txc_rls_service_t *service, const char* package)
{
	tsk_list_item_t *item = 0;
	if(service)
	{
		if(!(service->packages)) 
		{
			TSK_LIST_CREATE(service->packages);
		}
		
		TSK_LIST_ITEM_CREATE(item);
		item->data = (void*)tsk_strdup2(package);
		tsk_list_add_item(service->packages, &item);
	}
}

/**@ingroup txc_rls_group
* Internal function to free an rls service previously created using @ref TXC_RLS_SERVICE_CREATE.
* You MUST call @ref TXC_RLS_SERVICE_SAFE_FREE to free a service.
* @param _service The service to free.
* @sa @ref TXC_RLS_SERVICE_SAFE_FREE
*/
void txc_rls_service_free(void **_service)
{
	txc_rls_service_t **service = ((txc_rls_service_t**)_service);

	TSK_SAFE_FREE2((*service)->uri);
	TSK_SAFE_FREE2((*service)->resource_list);
	TSK_LIST_SAFE_FREE((*service)->packages);

	tsk_free2(_service);
}

/**@ingroup txc_rls_group
* Internal function to deserialize an rls service from an XML document.
* @param node The pointer to the XML node from which to deserialize the rls service
* @retval Pointer to a @ref txc_rls_service_t object or NULL if deserialization fail.
* You MUST call @ref TXC_RLS_SERVICE_SAFE_FREE to free the returned object.
*/
txc_rls_service_t* txc_rls_service_from_xml(const xmlNodePtr node)
{
	xmlNodePtr node2 = 0;
	txc_rls_service_t *rls_service = 0;
	tsk_list_item_t* item = 0;

	if(tsk_xml_find_node(node, "service", nft_none))
	{
		TXC_RLS_SERVICE_CREATE(rls_service);
				
		/* uri */
		node2 = tsk_xml_select_node(node, 
			TSK_XML_NODE_SELECT_ATT_VALUE("service", "uri"),
			TSK_XML_NODE_SELECT_END());
		rls_service->uri = tsk_strdup2(TSK_XML_NODE_SAFE_GET_TEXTVALUE(node2));

		/* resource-list */
		node2 = tsk_xml_select_node(node, TSK_XML_NODE_SELECT_BY_NAME("service"),
			TSK_XML_NODE_SELECT_BY_NAME("resource-list"),
			TSK_XML_NODE_SELECT_END());
		rls_service->resource_list = tsk_strdup2(TSK_XML_NODE_SAFE_GET_TEXTVALUE(node2));

		/* packages */
		node2 = tsk_xml_select_node(node, TSK_XML_NODE_SELECT_BY_NAME("service"),
			TSK_XML_NODE_SELECT_BY_NAME("packages"),
			TSK_XML_NODE_SELECT_CONTENT(),
			TSK_XML_NODE_SELECT_END());
		/* select first package */
		if(!tsk_xml_find_node(node2, "package", nft_none)) node2 = tsk_xml_find_node(node2, "package", nft_next);
		if(node2)
		{
			TSK_LIST_CREATE(rls_service->packages);
			do
			{
				TSK_LIST_ITEM_CREATE(item);
				item->data = tsk_strdup2(TSK_XML_NODE_SAFE_GET_TEXTVALUE(node2->children));
				tsk_list_add_item(rls_service->packages, &item);
			}
			while(node2 = tsk_xml_find_node(node2, "service", nft_next));
		}
	}else return 0;

	return rls_service;
}

/**@ingroup txc_rls_group
* Create a rls context from an XML buffer.
* @param buffer The XML buffer from which to create the rls context
* @param size The size of the XML buffer
* @retval Pointer to a @ref txc_rls_t object or NULL. You MUST call @ref txc_rls_free to free the returned object.
* @sa @ref txc_rls_free
*/
txc_rls_t* txc_rls_create(const char* buffer, size_t size)
{
	if(buffer && size)
	{
		txc_rls_t* rls = (txc_rls_t*)tsk_malloc2(sizeof(txc_rls_t));
		memset(rls, 0, sizeof(txc_rls_t));
		rls->docPtr = xmlParseMemory(buffer, (int)size);

		return rls;
	}

	return 0;
}

/**@ingroup txc_rls_group
* Get all rls services
* @param rls The rls context from which to extract the services
* @retval Pointer to a @ref txc_rls_service_L_t object or NULL. You must call @a TSK_LIST_SAFE_FREE to free the returned object.
*/
txc_rls_service_L_t* txc_rls_get_all_services(const txc_rls_t* rls)
{
	txc_rls_service_t* rls_service = 0;
	txc_rls_service_L_t* list = 0;
	xmlNodePtr node = 0;

	RLS_RETURN_IF_INVALID(rls);
	
	/* root */
	node = tsk_xml_select_node(rls->docPtr->children,
		TSK_XML_NODE_SELECT_BY_NAME("rls-services"),
		TSK_XML_NODE_SELECT_END());

	/* select first service */
	if(!tsk_xml_find_node(node, "service", nft_none)) node = tsk_xml_find_node(node, "service", nft_next);
	if(node)
	{
		TSK_LIST_CREATE(list);
		do
		{
			rls_service = txc_rls_service_from_xml(node);
			tsk_list_add_data(list, ((void**) &rls_service), txc_rls_service_free);
		}
		while(node = tsk_xml_find_node(node, "service", nft_next));
	}
	return list;
}

/**@ingroup txc_rls_group
* Serialize an rls service
* @param service The service to serialize
* @retval XML string representing the rls service.  You MUST call @a TSK_SAFE_FREE2 to free the returned string.
*/
char* txc_rls_service_serialize(const txc_rls_service_t *service)
{
	char* service_str = 0;
	char* package_str = 0;
	tsk_list_item_t* item = 0;

	if(!service) return 0;

	/* packages */
	tsk_list_foreach(item, service->packages)
	{
		char* curr = 0;
		tsk_sprintf(0, &curr, "<package>%s</package>", ((char*)item->data));
		tsk_strcat2(&package_str, curr);
		TSK_SAFE_FREE2(curr);
	}
	/* service */
	tsk_sprintf(0, &service_str,
				"<service uri=\"%s\">"
					"<resource-list>%s</resource-list>"
					"<packages>"
						"%s"
					"</packages>"
				"</service>",
				service->uri, service->resource_list, package_str);
	TSK_SAFE_FREE2(package_str);
	return service_str;
}

/**@ingroup txc_rls_group
* Serialize a list of several rls services
* @param services The list of rls services to serialize
* @retval XML string representing the list rls services.  You MUST call @a TSK_SAFE_FREE2 to free the returned string.
*/
char* txc_rls_services_serialize(const tsk_list_t *services)
{
	tsk_list_item_t* item = 0;
	char* services_str = 0;

	if(!services) return 0;

	/* xml header */
	tsk_strcat2(&services_str, RLS_XML_HEADER);

	tsk_list_foreach(item, services)
	{
		/* get service */
		txc_rls_service_t *service = ((txc_rls_service_t*)item->data);
		char* service_str = txc_rls_service_serialize(service);
		tsk_strcat2(&services_str, service_str);
		TSK_FREE(service_str);
	}
	
	/* xml footer */
	tsk_strcat2(&services_str, RLS_XML_FOOTER);

	return services_str;
}

/**@ingroup txc_rls_group
* Internal function to free an rls context previously created using @ref txc_rls_create.
* @param rls The context to free
* @sa @ref txc_rls_create
*/
void txc_rls_free(txc_rls_t **rls)
{
	if(*rls)
	{	
		xmlFreeDoc((*rls)->docPtr);
		
		tsk_free2(rls);
	}
}

#undef RLS_RETURN_IF_INVALID

#undef RLS_XML_HEADER
#undef RLS_XML_FOOTER