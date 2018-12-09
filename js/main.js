
var $form_elem = null;

$(document).ready(function(){
    var options = {
        onFail: function(){
            alert("您还有"+ $form_elem.getInvalid().length +'处必填项未完成！');
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
                data: { extension: ['jpg'] }
            },
            'comments': {
                filters: 'min max',
                data: { min: 30, max: 200 }
            },
            'states': {
                filters: 'exclude',
                data: { exclude: ['default'] },
                errors : {
                    exclude: '.'
                }
            },
            'langs[]': {
                filters: 'min max',
                data: { min: 2, max: 3 },
                errors: {
                    min: 'Check at least <strong>2</strong> options.',
                    max: 'No more than <strong>3</strong> options allowed.'
                }
            }
        }
    };
    addEventListener("load", function() { setTimeout(hideURLbar, 0); }, false); 
    function hideURLbar(){ window.scrollTo(0,1); } 
    $form_elem = $('#main-form').idealforms(options).data('idealforms');
    $('#reset').click(function(){
        $form_elem.reset().fresh().focusFirst();
    });
    $form_elem.focusFirst();
    var naviLang =(navigator.language || navigator.browserLanguage).toUpperCase();
    if(naviLang.indexOf("ZH") <= -1) {
        try{
            document.getElementsByTagName('section')[0].name = "Scroll to get more.";
        }catch(e){;}
        $("img").attr("title","QQ me");
        $(".ideal-label").html("Homepage:");
        $("#request").html("Submit");
        $("#reset").html("Reset");
    }
});

var formArray = new Array(
    "username",
    "passwd",
    "email",
    "date",
    "icon",
    "website",
    "zip",
    "comments"
);

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
        if(arrKey === "passwd"){
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
