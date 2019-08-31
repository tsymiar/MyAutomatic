var param = "";
var link = "trans/service.php?action=xxx";

let currentGroup = 1;
const LI_NUM_PER_PAGE = 10;
const ITEM_NUM_PER_PAGE = 20;

function show_prev_page() {
    if(currentGroup < 2)
        return;
    document.getElementById("li_prev").className = "active";
    if (currentGroup === 2)
        showPage(1, null);
    else
        showPage((currentGroup - 2) * LI_NUM_PER_PAGE + 1, true);
    setTimeout(function () {
        document.getElementById("li_prev").className = "";
    }, 100);
    currentGroup--;
}

function show_next_page(total) {
    if (currentGroup > (total / LI_NUM_PER_PAGE / ITEM_NUM_PER_PAGE)) {
        return;
    }
    document.getElementById("li_next").className = "active";
    showPage(currentGroup * LI_NUM_PER_PAGE + 1, true);
    setTimeout(function () {
        document.getElementById("li_next").className = "";
    }, 100);
    currentGroup++;
}

function showPage(page, group) {
    $("#h_page").val(page);
    $("#h_pnum").val(ITEM_NUM_PER_PAGE);
    setPopComponent(false);

    var limit = ITEM_NUM_PER_PAGE;
    var offset = (page - 1) * limit;

    nativeXMLHttp("POST", link, param, function(rsp_text){
        if (this.readyState === 4 && this.status === 200) {
            let html = "";
            let recordCount = 0;
            let json = JSON.parse(rsp_text);
            let item = json["items"];

            for (let i = 0; i < item.length; i++) {
                recordCount++;
            }
			
            let total_num = json["total"];
            $("#recordCount").html(total_num);
			
            let curr_page = page;
            let begin_page = 1;
            let count_page = LI_NUM_PER_PAGE;
            if (group) {
                if (page % LI_NUM_PER_PAGE === 0)
                    begin_page = (parseInt(page / LI_NUM_PER_PAGE) - 1) * LI_NUM_PER_PAGE + 1;
                else
                    begin_page = page - page % LI_NUM_PER_PAGE + 1;
                count_page = LI_NUM_PER_PAGE + begin_page - 1;
            }
            html = '<li id="li_prev" title="prev"><a onclick="show_prev_page();">‹‹</a></li>';
            for (let i = begin_page; i <= count_page; i++) {
                if ((i - 1) * ITEM_NUM_PER_PAGE >= total_num)
                    break;
                let active = "";
                if (i === curr_page) {
                    active = "class=\"active\"";
                }
                html += '<li id="li_page' + i + '" ' + active + ' style="cursor: pointer">'
                    + '<a onclick="showPage(' + i + ', true, );">' + i + '</a></li>';
            }
            html += '<li id="li_next" title="next">'
                + '<a onclick="show_next_page(' + total_num + ');">››</a></li>';
            page_elem.html(html);
        } else if (this.status === 401) {
            location.reload(true);
        }
    };
}
