var Base64Option = {
    Base64Encoding: 0,
    Base64UrlEncoding: 1,
    KeepTrailingEquals: 0,
    OmitTrailingEquals: 2,
}

var SortOrder = {
    AscendingOrder: 0,
    DescendingOrder: 1,
}

var ResizeMode = {
    Interactive: 0,
    Stretch: 1,
    Fixed: 2,
    ResizeToContents: 3,
    Custom: 2,
}

var NetworkReplyError = {
    Ok: 0,
    UnsupportedScheme: 1,
    Connection: 2,
    Connection400: 3,
    Connection401: 4,
    Connection403: 5,
    Connection404: 6,
    Connection4XX: 7,
    Connection5XX: 8,
    FileTooLarge: 9,
    Download: 10,
    Aborted: 11,
}

var PagesMode = {
    Single: 0,
    Multi : 1,
    List: 2,
}
var CompleterMode = {
    None: 0,
    Continuous: 1,
    All: 2,
}

var ItemDataRole = {
    UserRole: 0x0100,
}

%1

(function() { return {
    getInfo: getInfo,
    prepareWidget: prepareWidget,
    finalize: finalize,
    getQMPlay2Url: getQMPlay2Url,
    getSearchReply: getSearchReply,
    addSearchResults: addSearchResults,
    pagesMode: pagesMode,
    getPagesList: getPagesList,
    hasWebpage: hasWebpage,
    getWebpageUrl: getWebpageUrl,
    completerMode: completerMode,
    getCompleterReply: getCompleterReply,
    getCompletions: getCompletions,
    completerListCallbackSet: completerListCallbackSet,
    hasAction: hasAction,
    convertAddress: convertAddress,
}})()
