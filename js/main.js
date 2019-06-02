
var $form_elem = null;

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
            var tmp1 = arrVal;
            var tmp2 = tmp1.substr(tmp1.lastIndexOf("/") + 1);
            var tmp3 = tmp1.substring(0,tmp1.lastIndexOf("/"))
            var tmpVal = tmp2 + "/" + tmp3;
            console.log(tmp1+"\n"+tmp2+"\n"+tmp3+"\n"+tmpVal);
            arrVal = tmpVal.replace(/\//g,"-");
        }
        if(i === formArray.length-1){
            param += (arrKey + "=" + arrVal);
        }else{
            param += (arrKey + "=" + arrVal + "&");
        }
    }
    nativeXMLHttp("POST", link, param, function(rsp_text){
            if(rsp_text){
                if(rsp_text.indexOf("ERROR:") !== -1 || rsp_text.indexOf("error") !== -1 ||
                        rsp_text.indexOf("/<b>.*</b>/") !== -1){
                    setPopDivNoScroll("clazz_pop_div", "id_pop_div", true, rsp_text);
                }else{
                    var json = JSON.parse(rsp_text);
                    if(json.code === 200){
                        alert(json.message);
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
        more_elem.html("➤");
        $("#more-form").css("display", "none");
        more_elem.attr("title","more");
    } else {
        more_elem.html("✖");
        $("#more-form").css("display", "block");
        more_elem.attr("title","less");
    }
}