
var $form_elem = null;
var g_Interval = null;
let formdata = window.location.search.substring(1);

var formArray = [
    "username",
    "password",
    "email",
    "icon",
    "date",
    "website",
    "zip",
    "comments"
];

const ideal_errors = [
    jQuery.idealforms.errors.required,
    jQuery.idealforms.errors.digits,
    jQuery.idealforms.errors.number,
    jQuery.idealforms.errors.name,
    jQuery.idealforms.errors.username,
    jQuery.idealforms.errors.passwd,
    jQuery.idealforms.errors.strongpass,
    jQuery.idealforms.errors.email
];

var cnZh_errors = [
    '该处是必填的.',
    '必须是唯一的数字.',
    '必须是数字.',
    '至少有3个字符长，并且仅包含字母.',
    '长度必须在4到32个字符之间，并以字母开头.',
    '至少6个字符长，并且至少包含一个数字、一个大写字母和一个小写字母.',
    '至少为8个字符长，至少包含一个大写字母和一个小写字母和一个数字或特殊字符.',
    '必须是一个有效的E-mail地址. <br><em>(e.g. user@gmail.com)</em>'
];

jQuery.idealforms.errors.required = cnZh_errors[0];
jQuery.idealforms.errors.number = cnZh_errors[1];
jQuery.idealforms.errors.digits = cnZh_errors[2];
jQuery.idealforms.errors.name = cnZh_errors[3];
jQuery.idealforms.errors.username = cnZh_errors[4];
jQuery.idealforms.errors.passwd = cnZh_errors[5];
jQuery.idealforms.errors.strongpass = cnZh_errors[6];
jQuery.idealforms.errors.email = cnZh_errors[7];

$(document).ready(function() {
    var options = {
        onFail: function () {
            alert("您还有" + $form_elem.getInvalid().length + '处必填项未完成！');
        },
        inputs: {
            // look at line 41
            'user[name]': {
                filters: 'required username',
                data: {
                    //ajax: { url:'validate.php' }
                }
            },
            'file': {
                filters: 'extension',
                data: {extension: ['jpg']}
            },
            'comments': {
                filters: 'min max',
                data: {min: 30, max: 200}
            },
            'states': {
                filters: 'exclude',
                data: {exclude: ['default']},
                errors: {
                    exclude: '.'
                }
            },
            'langs[]': {
                filters: 'min max',
                data: {min: 2, max: 3},
                errors: {
                    min: 'Check at least <strong>2</strong> options.',
                    max: 'No more than <strong>3</strong> options allowed.'
                }
            }
        }
    };
    addEventListener("load", function () {
        setTimeout(hideURLbar, 0);
    }, false);

    function hideURLbar() {
        window.scrollTo(0, 1);
    }

    $form_elem = $('#main-form').idealforms(options).data('idealforms');
    $('#reset').click(function () {
        $form_elem.reset().fresh().focusFirst();
    });
    $form_elem.focusFirst();
    var naviLang = (navigator.language || navigator.browserLanguage).toUpperCase();
    if (naviLang.indexOf("ZH") <= -1) {
        jQuery.idealforms.errors.required = ideal_errors[0];
        jQuery.idealforms.errors.number = ideal_errors[1];
        jQuery.idealforms.errors.digits = ideal_errors[2];
        jQuery.idealforms.errors.name = ideal_errors[3];
        jQuery.idealforms.errors.username = ideal_errors[4];
        jQuery.idealforms.errors.passwd = ideal_errors[5];
        jQuery.idealforms.errors.strongpass = ideal_errors[6];
        jQuery.idealforms.errors.email = ideal_errors[7];
        try {
            document.getElementsByTagName('section')[0].name = "Scroll to show more.";
            $('.ideal-tabs-wrap').html(
                "<li class='ideal-tabs-tab ideal-tabs-tab-active'>" +
                "<span>Scroll to show more.</span>" +
                "<i class='ideal-tabs-tab-counter'>" + $('.ideal-tabs-tab-counter').text() +
                "</i></li>"
            )
        } catch (e) {
        }
        $("img").attr("title", "QQ me");
        var labels = [];
        for (var i = 0; i < formArray.length; i++) {
            var label = labels[i] = formArray[i];
            if (formArray[i] === "pass") {
                labels[i] = "Password";
            } else {
                labels[i] = labels[i][0].toLocaleUpperCase() + label.substring(1, label.length);
            }
            labels[i] += ":";
        }
        $(".ideal-label").each(function (i) {
            $(this).html(labels[i]);
        });
        $("#request").html("Submit");
        $("#reset").html("Reset");
    }
    if(formdata.indexOf("http://") !== -1 || formdata.indexOf("https://") !== -1) {
        setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, "<b><font size='2'>请稍等...<font><b>", 250);
        nativeXMLHttp("POST", "trans/service.php", 
        ("action=file_download&url=" + formdata), function(text) {
            try {
                var json = JSON.parse(text);
                if (json.errno === 201) {
                    let fileSize = "";
                    let action = "action=file_download&query=" + json.message;
                    g_Interval = setInterval(function(){
                            if(fileSize !== "" && action.indexOf("&file=") === -1 && action.indexOf("&size=") === -1)
                                action += fileSize;
                            nativeXMLHttp("POST", "trans/service.php", action, function(text, status){
                                try{
                                    if(text.indexOf("ERROR") !== -1 || text.indexOf("Warning") !== -1 || text.indexOf("Fatal error") !== -1){
                                        clearInterval(g_Interval);
                                    }
                                    var json = JSON.parse(text);
                                    if(json.errno === 200 || status === 303){
                                        clearInterval(g_Interval);
                                        setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, 
                                        (json.data?json.data.href+"click to download":json.message), 250, null, null, 80);
                                    }else if(json.errno === 201){
                                        action = "action=file_download&file=" + json.message;
                                    }else if(json.errno === 300){
                                        setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, json.message, 250);
                                        fileSize = (json.data ? "&size=" + json.data.size : "");
                                    }
                                }catch(e){
                                    setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, text + "<br>" + e);
                                }
                            });
                        },2400);
                }
            } catch (e) {
                setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, text + "<br>" + (text.indexOf("ERROR") == 0 ? "" : e));
            }
        });
    }
});

function click2Submit(){
    $("#dontjump").html("");
    if($form_elem === null || $form_elem.getInvalid().length >= 1){
        return;
    }
    var key = null;
    var icon = "";
    var param = "";
    var arrVal = "";
    var link = "trans/service.php?action=register";
    for(var i=0; i<formArray.length; i++){
        var arrKey = formArray[i];
        arrVal = $("#"+arrKey).val();
        if(arrKey === "username"){
            key = arrVal;
        }
        if(arrKey === "password"){
            if(key !== null){
                arrVal = aes_encrypt(arrVal,key);
            }
        }
        if(arrKey === "icon"){
            icon = arrVal = $(".ideal-file-filename").val();
        }
        if(arrKey === "date"){
            var tm1 = arrVal;
            var tm2 = tm1.substr(tm1.lastIndexOf("/") + 1);
            var tm3 = tm1.substring(0,tm1.lastIndexOf("/"))
            var tmVal = tm2 + "/" + tm3;
            arrVal = tmVal.replace(/\//g,"-");
        }
        if(i === formArray.length-1){
            param += (arrKey + "=" + arrVal);
        }else{
            param += (arrKey + "=" + arrVal + "&");
        }
    }
    nativeXMLHttp("POST", link, param, function(rsp_text){
            if(rsp_text){
                if(rsp_text.indexOf("ERROR") !== -1 || rsp_text.indexOf("error") !== -1 ||
                        rsp_text.indexOf("/<b>.*</b>/") !== -1 || rsp_text.indexOf("Warning") !== -1){
                    var star = rsp_text.indexOf("<b>/var") + 3;
                    var end_ = rsp_text.indexOf(".php");
                    var _end = rsp_text.substr(star, end_ - star);
                    var last = _end.lastIndexOf('/');
                    var find = rsp_text.substr(star, last);
                    setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, rsp_text.replace(find, "..."));
                }else{
                    try {
                        var json = JSON.parse(rsp_text);
                        if(json.errno === 200){
                            alert(json.message);
                        }
                    } catch (e) {
                        setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, rsp_text + "<br>" + e);
                    }
                }
            }
        });
    if(icon !== null && icon !== ""){
        UpladFile("icon", "trans/service.php?action=file_upload2");
    }
}

function showMoreform() {
    var more_elem = $("#more-link");
    if(more_elem.html() === "✖"){
        more_elem.html("<img src='image/arrow.svg' width='50%'>");
        $("#more-form").css("display", "none");
        more_elem.css("marginRight", "-10%");
        more_elem.attr("title","more");
    } else {
        more_elem.html("✖");
        more_elem.css("marginRight", "6%");
        $("#more-form").css("display", "block");
        more_elem.attr("title","less");
    }
}