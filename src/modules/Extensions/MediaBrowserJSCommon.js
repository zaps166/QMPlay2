const Base64Option = {
    Base64Encoding: 0,
    Base64UrlEncoding: 1,
    KeepTrailingEquals: 0,
    OmitTrailingEquals: 2,
}

const SortOrder = {
    AscendingOrder: 0,
    DescendingOrder: 1,
}

const ResizeMode = {
    Interactive: 0,
    Stretch: 1,
    Fixed: 2,
    ResizeToContents: 3,
    Custom: 2,
}

const NetworkReplyError = {
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

const PagesMode = {
    Single: 0,
    Multi : 1,
    List: 2,
}
const CompleterMode = {
    None: 0,
    Continuous: 1,
    All: 2,
}

const ItemDataRole = {
    UserRole: 0x0100,
}

%1

(() => { return {
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
