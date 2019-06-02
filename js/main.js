
var $form_elem = null;

var formArray = [
    "username",
    "password",
    "email",
    "date",
    "icon",
    "website",
    "zip",
    "comments"
];

jQuery.idealforms.errors.required = '该处是必填的.';
jQuery.idealforms.errors.number = '必须是数字.';
jQuery.idealforms.errors.digits = '必须是唯一的数字.';
jQuery.idealforms.errors.name = '至少有3个字符长，并且仅包含字母.';
jQuery.idealforms.errors.username = '长度在4到32个字符之间，并以字母开头.';
jQuery.idealforms.errors.passwd = '至少6个字符长，并且至少包含一个数字、一个大写字母和一个小写字母.';
jQuery.idealforms.errors.strongpass = '至少为8个字符长，至少包含一个大写字母和一个小写字母和一个数字或特殊字符.';
jQuery.idealforms.errors.email = '必须是一个有效的E-mail地址. <br><em>(e.g. user@gmail.com)</em>';

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